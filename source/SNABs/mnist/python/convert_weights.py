import argparse

parser = argparse.ArgumentParser(description='Converts a Keras model to json interpreted by the respective SNAB')
parser.add_argument('-a', help="JSON", required=True)
parser.add_argument('-w', help="Weights in HDF5", required=True)
parser.add_argument('-o', help="Ouput file name", required=True)
args = parser.parse_args()

from keras.models import Sequential, model_from_json
import json
import numpy as np 

from keras.constraints import Constraint
from keras import backend as K
class WeightClip(Constraint):
    '''Clips the weights incident to each hidden unit to be inside a range
    '''
    def __init__(self, c=2):
        self.c = c

    def __call__(self, p):
        return K.clip(p, -self.c, self.c)

    def get_config(self):
        return {'name': self.__class__.__name__,
                'c': self.c}

json_file = open(args.a).read()
model = model_from_json(json_file)
model.load_weights(args.w)
netw = json.loads(json_file)

data = {}
if netw["keras_version"][0] != "2":
    print("Warning: script was written for Keras 2.3.0")

data["netw"] = []
for ind, layer in enumerate(netw["config"]["layers"]):
    layer_dict = {}
    if(layer["class_name"] == "Dropout"):
        print("Ignoring dropout layer")
        continue
    elif(layer["class_name"] == "Flatten"):
        print("Ignoring Flatten layer")
        continue
    elif(layer["class_name"] == "AveragePooling2D"):
        print("Ignoring AveragePooling2D layer")
        continue
    elif(layer["class_name"] == "InputLayer"):
        print("Ignoring InputLayer layer")
        continue
    elif(layer["class_name"] == "Dense"):
        layer_dict["class_name"] = "Dense"
        layer_dict["size"] = layer["config"]["units"]
        try:
            layer_name = layer["config"]["name"]
            for layer_2 in model.layers:
                if layer_2.name == layer_name:
                    layer_dict["weights"] = layer_2.get_weights()[0].tolist()
                    break
        except:
            layer_dict["weights"] = model.layers[ind].get_weights()[0].tolist()
        # Weight[i][j]: i input, j output
    elif(layer["class_name"] == "Conv2D"):
        layer_dict["class_name"] = "Conv2D"
        layer_dict["size"] = layer["config"]["filters"]
        layer_dict["stride"] = layer["config"]["strides"][0]
        layer_dict["padding"] = layer["config"]["padding"]
        if "batch_input_shape" in layer["config"]:
            layer_dict["input_shape_x"] = layer["config"]["batch_input_shape"][1]
            layer_dict["input_shape_y"] = layer["config"]["batch_input_shape"][2]
            layer_dict["input_shape_z"] = layer["config"]["batch_input_shape"][3]
        else:
            layer_dict["input_shape_x"] = None
            layer_dict["input_shape_y"] = None
            layer_dict["input_shape_z"] = None
        try:
            layer_name = layer["config"]["name"]
            for layer_2 in model.layers:
                if layer_2.name == layer_name:
                    layer_dict["weights"] = layer_2.get_weights()[0].tolist()
                    break
        except:
            layer_dict["weights"] = model.layers[ind].get_weights()[0].tolist()
    elif(layer["class_name"] == "MaxPooling2D"):
        print("MaxPooling2D", layer)
        continue
    else:
        raise RuntimeError("Unknown layer type " + layer["class_name"] + "!")
    data["netw"].append(layer_dict)
    

if(args.o.endswith(".json")):
    with open(args.o, 'w') as file:
        json.dump(data, file)
        
elif(args.o.endswith(".msgpack")):
    import msgpack
    with open(args.o, 'wb') as file:
        msgpack.dump(data, file, use_single_float=True)
else:
    raise RuntimeError("Wrong file name! File must end with either .json or .msgpack!")
