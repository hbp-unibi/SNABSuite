# With this script we can extract the weights of the fully connected network
# and convert it into our format
# Data from: 
# https://github.com/dannyneil/spiking_relu_conversion
# Diehl, P.U. and Neil, D. and Binas, J. and Cook, M. and Liu, S.C. and 
# Pfeiffer, M. Fast-Classifying, High-Accuracy Spiking Deep Networks Through 
# Weight and Threshold Balancing, 
# IEEE International Joint Conference on Neural Networks (IJCNN), 2015

import scipy.io 
mat_data = scipy.io.loadmat("nn_98.84.mat") 

data = {}
data["netw"] = []
for layer in mat_data['nn'][0][0][13][0]:
    layer_dict = {}
    layer_dict["class_name"] = "Dense"
    layer_dict["size"] = len(layer)
    layer_dict["weights"] = layer.transpose().tolist()
    data["netw"].append(layer_dict)

import msgpack
with open("netw_diehl.msgpack", 'wb') as file:
        msgpack.dump(data, file, use_single_float=True)
