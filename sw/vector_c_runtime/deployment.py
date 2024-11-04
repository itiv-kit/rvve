# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

import tensorflow as tf
from tensorflow import keras
import numpy as np
from fxpmath import Fxp
import re
import math
import os
import shutil
import tflite

#from torch import tensor

# The frontend import step is taken directly from the tflite frontend
# Codegen is done specifically for the designed kernels

class TensorWrapper:
    def __init__(self,  tensor_idx, tensor, buffer, quant_params):
        self.tensor_idx = tensor_idx
        self.tensor = tensor
        self.buffer = buffer
        self.quant_params = quant_params
        self.data = []

class TFLiteExporter:
    def __init__(self, model_path, out_path) -> None:
        self.file_path = model_path
        self.out_path = out_path
        self.out_path_param = "model/params.h"
        self.out_path_model = "model/model.h"

        self.interpreter = tf.lite.Interpreter(self.file_path)
        self.interpreter.allocate_tensors()

        with open(self.file_path, "rb") as f:
            tflite_model_buf = f.read()

        try:
            import tflite
            self.model = tflite.Model.GetRootAsModel(tflite_model_buf, 0)
        except AttributeError:
            import tflite.Model
            self.model = tflite.Model.Model.GetRootAsModel(tflite_model_buf, 0)

        assert self.model.SubgraphsLength() == 1,  "Only one subgraph is supported"
        self.subgraph = self.model.Subgraphs(0)

        self.builtin_op_codes = {}
        for key in dir(tflite.BuiltinOperator()):
            if not key.startswith("_"):
                val = getattr(tflite.BuiltinOperator(), key)
                if isinstance(val, int):
                    self.builtin_op_codes[val] = key

        self.params_codestream = ""
        self.model_codestream = ""
        self.model_codestream_workspace = ""

        self.supported_ops = {
            "FULLY_CONNECTED": self.export_dense,
            "CONV_2D": self.export_conv2d,
            "MAX_POOL_2D": self.export_maxpool2d,
        }

    def get_tensor_type_as_numpy(self, tensor_wrapper):
        """Returns np.dtype out of TensorType"""
        assert isinstance(tensor_wrapper, TensorWrapper)

        try:
            from tflite.TensorType import TensorType

            return {
                TensorType.UINT8: np.uint8,
                TensorType.INT8: np.int8,
                TensorType.INT16: np.int16,
                TensorType.FLOAT16: np.float16,
                TensorType.FLOAT32: np.float32,
                TensorType.INT32: np.int32,
                TensorType.INT64: np.int64,
                TensorType.BOOL: np.bool_,
            }[tensor_wrapper.tensor.Type()]
        except ImportError:
            raise ImportError("The tflite package must be installed")
        except KeyError:
            raise NotImplementedError(
                f"Tensor type '{tensor_wrapper.tensor.Type()}' currently not supported"
            )

    def get_tensor_shape(self, tensor_wrapper):
        """Returns tensor shape. Infers shape if the shape is empty."""
        assert isinstance(tensor_wrapper, TensorWrapper), "Expecting TensorWrapper here"
        if tensor_wrapper.tensor.ShapeLength() > 0:
            return tensor_wrapper.tensor.ShapeAsNumpy()
        else:
            raise Exception("No shape defined")

    def get_tensor_value(self, tensor_wrapper):
        """Get tensor buffer value from given tensor wrapper"""
        assert isinstance(tensor_wrapper, TensorWrapper)

        dtype = self.get_tensor_type_as_numpy(tensor_wrapper)
        data = tensor_wrapper.buffer.DataAsNumpy()

        if tensor_wrapper.tensor.ShapeLength() != 0:
            shape = [int(x) for x in (self.get_tensor_shape(tensor_wrapper))]
        else:
            shape = []

        return np.frombuffer(data, dtype=dtype).reshape(shape)

    def get_tensors(self, tensors_idx_list):
        """Get tensor wrapper list from given TFLite tensor index list"""
        return_list = list()
        for tensor_idx in tensors_idx_list:
            if tensor_idx < 0:
                return_list.append(TensorWrapper(tensor_idx, 0, 0))
                continue

            tensor = self.subgraph.Tensors(tensor_idx)
            buffer_idx = tensor.Buffer()
            buffer = self.model.Buffers(buffer_idx)

            # Check if the tensors are quantized. Parse if yes.
            qnn_params = None
            tflite_qnn_params = tensor.Quantization()
            if tflite_qnn_params is not None:
                # TFLite supports both per-tensor and per-axis (aka channel) quantization.  For
                # per-tensor quantization, scale and zero points are scalar values.  For per-axis
                # quantization, scale and zero points for the weights are tensors (activations are
                # per-tensor quantized). However, the TFLite quantization spec puts restrictions on
                # zero points for per-axis quantization.  Specifically, the zero point is a tensor
                # but all values are 0. More information can be found here -
                # https://www.tensorflow.org/lite/performance/quantization_spec

                tflite_scale = tflite_qnn_params.ScaleAsNumpy()
                tflite_zero_point = tflite_qnn_params.ZeroPointAsNumpy()
                is_qnn_params_valid = True

                # Handle Per-axis and per-tensor cases
                if isinstance(tflite_scale, np.ndarray):
                    assert isinstance(tflite_zero_point, np.ndarray)

                    # Tensor - Per-axis quantization
                    if tflite_scale.size != 1 and tflite_zero_point.size != 1:
                        raise NotImplementedError(" Per-axis quantization is not supported")

                    # Scalar - Per-tensor quantization
                    elif tflite_scale.size == 1 and tflite_zero_point.size == 1:
                        scale = float(tflite_scale[0])
                        zero_point = int(tflite_zero_point[0])

                    else:
                        raise NotImplementedError(
                            f"Quantized type {type(tflite_scale)} (scale) and  "
                            f"{type(tflite_zero_point)} (zero point) not supported"
                        )
                elif tflite_scale == 0 and tflite_zero_point == 0:
                    # Handle corner case for ops like quantized reshape whose second operand (shape)
                    # has zero scale and zero zero point. This is not used.
                    is_qnn_params_valid = False
                else:
                    raise NotImplementedError(f"Quantized type {type(tflite_scale)} not supported")

                # Check that the scale and zero points are valid.
                if is_qnn_params_valid:
                    qnn_params = dict()
                    qnn_params["scale"] = scale
                    qnn_params["zero_point"] = zero_point
            else:
                raise NotImplementedError("Non-quantized tensor detected. Only quantized ops are supported")
            return_list.append(TensorWrapper(tensor_idx, tensor, buffer, qnn_params))
        return return_list

    def get_input_tensors(self, op):
        ins = op.InputsAsNumpy()
        return self.get_tensors(ins)

    def get_output_tensors(self, op):
        out = op.OutputsAsNumpy()
        return self.get_tensors(out)

    def get_op_code_str(self, op):
        """Get TFLite ops string representation"""
        try:
            from tflite.BuiltinOperator import BuiltinOperator
        except ImportError:
            raise ImportError("The tflite package must be installed")

        op_code_list_idx = op.OpcodeIndex()

        op_c = self.model.OperatorCodes(op_code_list_idx)
        # In TFlite 2.4.x there was a change where the type of the field that contained
        # the builtin code changed from int8 to int32 in the flat buffer representation.
        # However, to retain support for old flat buffers that were created, they retained
        # the original 8 bit field, but named it "deprecated_builtin_code" in TFLite 2.4.
        # This means that the API function BuiltinCode() which originally returned the value
        # of the 8 bit field would now look for the value in the new int32 field in the
        # schema and DeprecatedBuiltinCode() will look at the old 8 bit field.
        # In TFLite 2.4, if the opcode value is less than 127, it can be in either field
        # (however, if it is only in the "builtin_code" field, the model is not backward
        # compatible), so similarly to TFLite 2.4 reader, we'll pick the higher value of the
        # two fields.
        # Remember however that this value came into existence only after Tensorflow
        # lite 2.4.x and hence encase it in a try -except block.
        # Phew !
        try:
            opc = max(op_c.DeprecatedBuiltinCode(), op_c.BuiltinCode())
        except AttributeError:
            # In versions before 2.4 the int8 field that holds the builtin code is accessed
            # by BuiltinCode() and DeprecatedBuiltinCode() doesn't exist
            opc = op_c.BuiltinCode()

        op_code_id = opc
        try:
            op_code_str = self.builtin_op_codes[op_code_id]
        except KeyError:
            raise NotImplementedError(
                "TFLite operator with code "
                + str(op_code_id)
                + " is not supported by this version of the fbs schema."
            )
        if op_code_id == BuiltinOperator.CUSTOM:
            # Custom operator
            custom_op_code_str = self.model.OperatorCodes(op_code_list_idx).CustomCode()

            if self.allow_custom_ops:
                return "CUSTOM"

            if custom_op_code_str == b"TFLite_Detection_PostProcess":
                return "DETECTION_POSTPROCESS"

            raise NotImplementedError("Custom operators are currently not supported")
        return op_code_str

    def get_tensor_type_str(self, tensor_type):
        """Get tensor type string representation when given TFLite tensor type"""
        try:
            from tflite.TensorType import TensorType
        except ImportError:
            raise ImportError("The tflite package must be installed")

        if tensor_type == TensorType.INT8:
            return "int8"
        if tensor_type == TensorType.INT16:
            return "int16"
        if tensor_type == TensorType.UINT8:
            return "uint8"
        if tensor_type == TensorType.FLOAT16:
            return "float16"
        if tensor_type == TensorType.FLOAT32:
            return "float32"
        if tensor_type == TensorType.INT32:
            return "int32"
        if tensor_type == TensorType.INT64:
            return "int64"
        if tensor_type == TensorType.BOOL:
            return "bool"
        raise NotImplementedError(f"Tensor type {str(tensor_type)} is currently not supported")

    def tensor_name(self, tensor: TensorWrapper):
        name = self.subgraph.Tensors(tensor.tensor_idx).Name()
        if name is not None:
            name = name.decode("utf-8")
        else:
            name = "autocode_tensor_" + str(tensor.tensor_idx)
        name_sanitized = re.sub('[^0-9a-zA-Z]+', '_', name)
        return name_sanitized

    def get_pad_value(data, kernel, stride):
        """Get the pad tuple of value for SAME padding

        Parameters
        ----------
        data:
            1D input data

        kernel:
            1D input kernel

        stride:
            1D input stride

        Returns
        -------
            pad tuple of value
        """

        out = int(math.ceil(float(data) / float(stride)))
        pad = max(0, (out - 1) * stride + kernel - data)
        pad_before = pad // 2
        pad_after = pad - pad_before
        return pad_before, pad_after

    def emit_func_call(self, name, *args):
        # emit destination-passing-style function call
        call = f"\t{name}("
        for idx, a in enumerate(args):
            call += f"{a}"
            if idx < len(args) - 1:
                call += ", "
        call += ");\n"
        return call

    def export_c_header(self, input=None):
        self.params_codestream = "#ifndef MODEL_DATA_H\n"
        self.params_codestream += "#define MODEL_DATA_H\n\n"
        self.params_codestream += "#include <stdint.h>\n"
        self.params_codestream += "#include <stdlib.h>\n\n"
        self.params_codestream += "#include \"kernels.h\"\n\n"

        self.model_codestream = "#ifndef MODEL_H\n"
        self.model_codestream += "#define MODEL_H\n\n"
        self.model_codestream += "#include <stdint.h>\n"
        self.model_codestream += "#include <stdlib.h>\n"
        self.model_codestream += "#include \"params.h\"\n"
        self.model_codestream += "#include \"kernels.h\"\n\n"

        self.model_codestream += f"void execute_model(int8_t* in_data){{\n"
        
        # Extract information from the subgraph
        for idx in range(self.subgraph.OperatorsLength()):
            op = self.subgraph.Operators(idx)
            op_code = self.get_op_code_str(op)

            # code gen for supported ops
            assert op_code in self.supported_ops.keys(), f"Operation {op_code} not supported."
            self.supported_ops[op_code](op)

        self.params_codestream += "#endif\n"
        
        self.model_codestream += "}\n"
        self.model_codestream += "#endif\n"

        os.makedirs("model", exist_ok=True)
        #shutil.copy("kernels.h", "model/kernels.h")
        with open(self.out_path_param, "w") as f:
            f.write(self.params_codestream)
        with open(self.out_path_model, "w") as f:
            f.write(self.model_codestream)

    def export_dense(self, op):
        inputs = self.get_input_tensors(op)
        outputs = self.get_output_tensors(op)

        assert len(inputs) == 3, "Expected to find three inputs on fully-connected layer. Does your layer have bias?"

        input = inputs[0]
        weight = inputs[1]
        bias = inputs[2]
        out = outputs[0]

        weights_value = self.get_tensor_value(inputs[1])
        self.params_codestream = CodeGenC.write_tensor(weight, weights_value, self.tensor_name(inputs[1]), self.params_codestream)

        # Modify bias value with precomputations
        # Note: dense layer will be computed as X @ W.T + b
        bias_value = self.get_tensor_value(inputs[2])

        out_scale = out.quant_params["scale"]
        out_zp = out.quant_params["zero_point"]
        bias_scale = bias.quant_params["scale"]
        bias_zp = bias.quant_params["zero_point"]
        wght_scale = weight.quant_params["scale"]
        wght_zp = weight.quant_params["zero_point"]
        in_scale = input.quant_params["scale"]
        in_zp = input.quant_params["zero_point"]

        assert wght_zp == 0, f"Currently only symmetric quantization for weights is supported but found zp_weights={wght_zp}"

        scale = (in_scale * wght_scale) / out_scale

        self.params_codestream = CodeGenC.write_fixed_point(
                                    self.tensor_name(out) + "_scaling",
                                    scale,
                                    self.params_codestream,
                                    )
        
        bias_mod = (out_zp + (bias_scale/out_scale) * (bias_value.astype(np.int32) - bias_zp)
                    - scale * in_zp * np.sum(weights_value.transpose().astype(np.int32), axis=0, keepdims=True))
        bias_mod = bias_mod.astype(np.int32)[0]
        self.params_codestream = CodeGenC.write_tensor(bias, bias_mod, self.tensor_name(bias), self.params_codestream)

        in_shape = self.get_tensor_shape(input)
        weight_shape = self.get_tensor_shape(weight)

        self.model_codestream += self.emit_func_call(
            "tiled_matmul_qnn",
            self.tensor_name(input),
            self.tensor_name(weight),
            self.tensor_name(out),
            in_shape[0],
            in_shape[1],
            weight_shape[1],
            self.tensor_name(out)+"scaling",
            "true", #b_is_transposed
        )


    def export_conv2d(self, op):
        from tflite.Conv2DOptions import Conv2DOptions
        from tflite.Padding import Padding

        inputs = self.get_input_tensors(op)
        outputs = self.get_output_tensors(op)

        assert len(inputs) == 3, "Expected to find three inputs on convolution layer. Does your layer have bias?"

        ifm = inputs[0]
        weights = inputs[1]
        bias = inputs[2]
        out = outputs[0]

        options = op.BuiltinOptions()
        conv_options = Conv2DOptions()
        conv_options.Init(options.Bytes, options.Pos)

        stride_h = conv_options.StrideH()
        stride_w = conv_options.StrideW()
        padding = conv_options.Padding()

        fused_activation_function = conv_options.FusedActivationFunction()
        assert fused_activation_function in [tflite.ActivationFunctionType.NONE, tflite.ActivationFunctionType.RELU]

        batch_size, input_h, input_w, ic = [int(x) for x in self.get_tensor_shape(ifm)]

        # Adapt kernel layout
        # Default TFLite is OHWI but we need HWOI
        oc, kh, kw, ic = [int(x) for x in self.get_tensor_shape(weights)]
        weights_value = self.get_tensor_value(weights)
        weights_value = weights_value.transpose((1, 2, 0, 3))

        self.params_codestream = CodeGenC.write_tensor(weights, weights_value, self.tensor_name(weights), self.params_codestream)

        _, out_h, out_w, oc = [int(x) for x in self.get_tensor_shape(weights)]

        if padding == Padding.VALID:
            pad = 0
        elif padding == Padding.SAME:
            pad_top, pad_bottom = self.get_pad_value(input_h, kh, stride_h)
            pad_left, pad_right = self.get_pad_value(input_w, kw, stride_w)
            assert pad_top == pad_bottom == pad_left == pad_right, "Only same padding in all directions is supported"
            pad = pad_top
        
        # Modify bias to account for quantization
        bias_value = self.get_tensor_value(bias)

        out_scale = out.quant_param["scale"]
        out_zp = out.quant_param["zero_point"]
        bias_scale = bias.quant_param["scale"]
        bias_zp = bias.quant_param["zero_point"]
        wght_scale = weights.quant_param["scale"]
        wght_zp = weights.quant_param["zero_point"]
        in_scale = ifm.quant_param["scale"]
        in_zp = ifm.quant_param["zero_point"]

        assert wght_zp == 0, f"Currently only symmetric quantization for weights is supported but found zp_weights={wght_zp}"

        scale = (in_scale * wght_scale) / out_scale
        
        bias_mod = (out_zp + (bias_scale/out_scale) * (bias_value.astype(np.int32) - bias_zp)
                    - scale * in_zp * np.sum(weights_value.astype(np.int32), axis=[0,1,2], keepdims=True))
        bias_mod = bias_mod.astype(np.int32)
        breakpoint()
        self.params_codestream = CodeGenC.write_tensor(bias, bias_mod, self.tensor_name(bias), self.params_codestream)

        self.model_codestream += self.emit_func_call(
            "tiled_conv_qnn",
            self.tensor_name(ifm),
            self.tensor_name(weights),
            self.tensor_name(out),
            self.tensor_name(bias_mod),
            batch_size, input_h, input_w, ic,
            out_h, out_w, oc,
            kh, kw, 
            pad, stride_h,
            self.tensor_name(out)+"scaling",
            in_zp,
        )

    def export_maxpool2d(self, op):
        from tflite.Pool2DOptions import Pool2DOptions
        from tflite.Padding import Padding

        inputs = self.get_input_tensors(op)
        assert(len(inputs == 1), "maxpool2d can only have one input tensor")

        outputs = self.get_output_tensors(op)
        assert(len(outputs == 1), "maxpool2d can only have one output tensor")

        input = inputs[0]
        out = outputs[0]

        assert(input)

        op_options = op.BuiltinOptions()
        pool2d_options = Pool2DOptions()
        pool2d_options.Init(op_options.Bytes, op_options.Pos)
        stride_h = pool2d_options.StrideH()
        stride_w = pool2d_options.StrideW()
        assert(stride_h == stride_w, "Only equal striding in all dimensions is currently supported")

        padding = pool2d_options.Padding()
        filter_h = pool2d_options.FilterHeight()
        filter_w = pool2d_options.FilterWidth()

        batch_size, input_h, input_w, channels = [int(x) for x in self.get_tensor_shape(input)]
        _, output_h, output_w, _ = [int(x) for x in self.get_tensor_shape(out)]

        if padding == Padding.VALID:
            pad = 0
        elif padding == Padding.SAME:
            pad_top, pad_bottom = self.get_pad_value(input_h, filter_h, stride_h)
            pad_left, pad_right = self.get_pad_value(input_w, filter_w, stride_w)
            assert pad_top == pad_bottom == pad_left == pad_right, "Only same padding in all directions is supported"
            pad = pad_top

        self.model_codestream += self.emit_func_call(
            "maxpool",
            self.tensor_name(input),
            self.tensor_name(out),
            batch_size,
            output_h,
            output_w,
            channels,
            filter_h,
            filter_w,
            pad,
            stride_h,
        )


class CodeGenC:
    def __init__(self):
        pass

    @staticmethod
    def to_fixed_point(scale, n_word = 64, n_frac=32):
        scale_fp = Fxp(scale, signed=True, n_word=n_word, n_frac=n_frac) 
        return scale_fp

    @staticmethod
    def write_tensor(tensor, data, tensor_name, codestream):
        if tensor.buffer.DataIsNone():
            return codestream

        shape = tensor.tensor.ShapeAsNumpy()
        shape_str = "" 
        for s in shape:
            shape_str += f"[{s}]"

        dtype = data.dtype #tensor.buffer.DataAsNumpy().dtype
        if dtype in ["int32", "np.int32"]:
            dtype_string = "int32_t"
        elif dtype in ["int8", "np.int8"]:
            dtype_string = "int8_t"
        elif dtype in ["int16", "np.int16"]:
            dtype_string = "int32_t" #accelerator doesn't support int16, so we just cast here
        elif dtype in ["uint8", "np.uint8"]:
            dtype_string = "uint8_t"
        else:
            raise Exception(f"Unsupported tensor datatype: {dtype}")

        # Decide which type of tensor we want to print here
        # As a rule we want to have the innermost dimension on one line in the output file
        # TODO: We could also just print the flattended version
            
        codestream += f"{dtype_string} {tensor_name}{shape_str} = {{"

        #data_flattened = data.flatten()

        # 1D Tensor
        if(len(shape) == 1):
            for i in range(0, shape[0]):
                if i != shape[0]-1:
                    codestream += f"{str(data[i])}, "
                else:                    
                    codestream += f"{str(data[i])}"
            codestream += f"}};\n"

        # 2D Tensor
        elif(len(shape) == 2):
            for outer in range(0, shape[0]):
                codestream += f"\t{{"
                for inner in range(0, shape[1]):
                    if inner != shape[1]-1:
                        codestream += f"{str(data[outer][inner])}, "
                    else:
                        codestream += f"{str(data[outer][inner])}"
                codestream += f"}},\n"
            codestream += f"}};\n"

        # 3D Tensor
        elif (len(shape) == 3):
            for i in range(0, shape[0]):
                codestream += "\t{{"
                for j in range(0, shape[1]):
                    codestream += "\t\t{"
                    for k in range(0, shape[2]):
                        codestream += "\t\t\t{{"
                        if k != shape[2]-1:
                            codestream += f"{str(data[i][j][k][l])}, "
                        else:
                            codestream += f"{str(data[i][j][k][l])}"
                    codestream += f"\t\t\t}}\n"
                codestream += f"\t\t}}\n"
            codestream += f"\t}};\n"

        elif (len(shape) == 4):
            for i in range(0, shape[0]):
                codestream += "\t{{"
                for j in range(0, shape[1]):
                    codestream += "\t\t{"
                    for k in range(0, shape[2]):
                        codestream += "\t\t\t{{"
                        for l in range(0, shape[3]):
                            codestream += "\t\t\t\t{{"
                            if l != shape[3]-1:
                                codestream += f"{str(data[i][j][k][l])}, "
                            else:
                                codestream += f"{str(data[i][j][k][l])}"
                        codestream += f"\t\t\t\t}}\n"
                    codestream += f"\t\t\t}}\n"
                codestream += f"\t\t}}\n"
            codestream += f"\t}};\n"
                    
        else:
            raise Exception("unsupported shape")

        codestream += "\n"
        return codestream


    @staticmethod
    def write_quantization_params(name, params, codestream):
        scale = params["scale"][0]
        zero_point = params["zero_point"][0]

        scale_fp = CodeGenC.to_fixed_point(scale)

        params_zero_point_str = f"{name}.zero_point"
        params_scale_val_str = f"{name}.scaling_factor.value"
        params_scale_fraction_len_str = f"{name}.scaling_factor.fraction_length"

        codestream += f"quantization_parameters_t {name+'_quant_param'};\n"
        codestream += f"{params_scale_val_str} = 0b{scale_fp.bin()}; //{scale_fp} \n"
        codestream += f"{params_scale_fraction_len_str} = {scale_fp.n_frac};\n"
        codestream += f"{params_zero_point_str} = {zero_point};\n\n"

        return codestream

    @staticmethod
    def write_fixed_point(name, val, codestream, n_word=64, n_frac=32):
        val_fp = Fxp(val, signed=True, n_word=n_word, n_frac=n_frac)
        n_int = n_word - 1 - n_frac
        dtype_str = "fixed_point_t"
        
        codestream += f"{dtype_str} {name} = {{ .value = 0b{val_fp.bin()}, .fraction_length = {n_frac}, .word_length = {n_word}, .int_length = {n_int} }};\n\n"

        return codestream

#class LSTMExporter(CodeGenC):
#    def __init__(self, file_path):
#        super().__init__()
#        self.file_path = file_path
#
#        self.input_details = None
#        self.output_details = None
#        self.tensors = None
#
#        self.codestream = ""
#
#        # Indices at which tensors are located after TFLite conversion
#        self.tensor_index_lookup = {
#            "input_input_x": 0,
#
#            "input_data_bias": 1,
#            "cell_gate_bias": 2,
#            "output_gate_bias": 3,
#            "forget_gate_bias": 4,
#
#            "input_input_weights": 5,
#            "input_forget_weights": 6,
#            "input_cell_weights": 7,
#            "input_output_weights": 8,
#            
#            "recurrent_input_weights": 9,
#            "recurrent_forget_weights": 10,
#            "recurrent_cell_weights": 11,
#            "recurrent_output_weights": 12,
#            
#            "output_state_in": 13,
#            "cell_state_in": 14
#        }
#
#        # We need to calculate the scaling parameters here:
#        #   For arrays with one value we return array[0]
#        #   For arrays with two values we calculate array[0]*array[1]
#        #   For arrays with three values we calculate (array[0]*array[1])/array[2]
#        self.tensor_quant_param_calc_lookup = {
#            "X": ["input_input_x"],
#
#            "B_input": ["input_data_bias"],
#            "B_forget": ["forget_gate_bias"],
#            "B_cell": ["cell_gate_bias"],
#            "B_output": ["output_gate_bias"],
#
#            "Hidden_State": ["output_state_in"],
#
#            "total_scaling_input": ["input_input_x", "input_input_weights"],
#            "total_scaling_forget": ["input_input_x", "input_forget_weights"],
#            "total_scaling_cell": ["input_input_x", "input_cell_weights"],
#            "total_scaling_output": ["input_input_x", "input_output_weights"],
#            
#            "total_scaling_recurrent_input": ["output_state_in", "recurrent_input_weights"],
#            "total_scaling_recurrent_forget": ["output_state_in", "recurrent_forget_weights"],
#            "total_scaling_recurrent_cell": ["output_state_in", "recurrent_cell_weights"],
#            "total_scaling_recurrent_output": ["output_state_in", "recurrent_output_weights"],
#
#            "total_scaling_cell_update1": ["logistic_scale", "cell_state_in", "cell_state_in"],
#            "total_scaling_cell_update1": ["tanh_scale", "logistic_scale", "cell_state_in"],
#
#            "total_scaling_hidden_update": ["cell_state_in", "tanh_scale", "output_state_in"]
#        }
#
#
#    def extract_tensors(self):
#        #converter = tf.lite.TFLiteConverter.from_saved_model(self.file_path)
#        interpreter = tf.lite.Interpreter(self.file_path)
#        interpreter.allocate_tensors()
#        self.input_details = interpreter.get_input_details()
#        self.output_details = interpreter.get_output_details()
#        self.tensor_details = interpreter.get_tensor_details()
#
#        #Create attributes with the tensors for each weight/bias
#        for name, idx in self.tensor_index_lookup.items():
#            zero_points = self.tensor_details[idx]["quantization_parameters"]["zero_points"]
#            scales = self.tensor_details[idx]["quantization_parameters"]["scales"]
#            assert(len(zero_points) == 1 and len(scales) == 1, "Model was not quantized tensorwise but channelwise, this is currently not supported!")
#
#            zero_point = zero_points[0]
#
#            #For scale we need to convert it into FixedPoint format. First, calculate n_frac and n_int depending on value
#            n_word = 32
#            n_int = int(np.ceil(np.log2(scales[0])))
#            n_int = max(1, n_int)
#            n_frac = n_word - n_int
#            
#            assert(n_frac <= 31, "Number of fractional bits can't be satisfied: n_frac <= 31 is not fulfilled")
#            assert(n_int >= 1, "n_int must be at least one")
#            scale = Fxp(scales[0], signed=False, n_word=n_word, n_frac=n_frac)
#
#            tensor = interpreter.get_tensor(idx)
#
#            #Handle special case of input tensor
#            if(len(tensor.shape) == 3 and tensor.shape[0] == 1):
#                tensor = np.squeeze(tensor, axis=0)
#
#            setattr(self, name, tensor)
#            setattr(self, name + "_zp", zero_point)
#            setattr(self, name + "_s", scale)
#
#
#    def write_c_header(self, codestream):
#
#        #for all tensors: write their contents to the codestream
#        for name in self.tensor_index_lookup.keys():
#            super().write_tensor(getattr(self, name), name, codestream)
#
#        # Write zero point and scale parameters to header
#        for name, params in self.tensor_quant_param_calc_lookup.items():
#            super().write_quantization_params(name, params, codestream)
#        
#        
#    def codegen(self):
#        self.extract_tensors()
#        self.export_c_header()
#
#
#class DenseExporter(CodeGenC):
#    def __init__(self, file_path):
#        self.file_path = file_path
#
#        self.input_details = None
#        self.output_details = None
#        self.tensors = None
#
#        self.tensor_index_lookup = {
#            "input_x": 0,
#            "weight": 1,
#            "bias": 2,
#            "out": 3,
#        }
#
#    def codgen(self, codestream):
#        pass
#
#
if __name__ == "__main__":
    exporter = TFLiteExporter("dense_model.tflite", "model.h")
    exporter.export_c_header()









