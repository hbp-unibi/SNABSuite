# Preparing a Deep Neural Network for spike conversion and benchmarking
This document describes the procedure to train an artificial neural network and prepare it for the conversion to a spiking neural network.

## Installing Tensorflow and Keras

On a linux system, prepare the software environemnt by executing

```bash
viurtualenv tensorflow
source tensorflow/bin/activate
pip install tensorflow
pip install keras
```

## Create a DNN and train it

Create a DNN as usual using keras. Please do not use any biases or other activation functions as ReLU, as this might impede the results of the conversion. 
After training, please make use of the keras integrated saving mechanism as shown in the examples files, e.g. using the following code 

```Python
# model is train keras model keras.model.Sequential()
model.save_weights('dnn_weights.h5')
json_string = model.to_json()
with open('dnn_model.json', 'w') as file:
    file.write(json_string)
```

## Preparing the data to be read by SNABSuite

For data conversion, there is a script called `convert_weights.py`.
In this case, it can be used like
```bash
python3 convert_weights.py dnn_model.json -w dnn_weights.h5 -o dnn_out.msgpack
```

To use the created model in SNABSuite, copy the model to build folder, and adapt one of the configs (e.g. config/MnistDiehl.json):

* For your target simulator, adapt the `dnn_file` entry to `dnn_out.msgpack`.
* If you used AveragePooling to scale down the image (for the Spikey system), set the `scaled_image` to true, otherwise to false
* Depending on the result you might have to tune the `max_weight` parameter

## Including new layer types

To add new layer types, there are several places that have to adapted: 

* `convert_weights.py` has to serialize the new layer type in a convenient way
* `mnist_mlp.hpp` The constructor of the MLP class reads in data for MLPs only and has to be extended. Depending on the new layer type, it might be more reasonable to create a new class, and rewrite this constructor. A different option is to convert the new layer type (e.g. conv) to a dense matrix. 
* `MNIST_BASE::create_deep_network`  in `mnist.cpp` implements the conversion from MLP to a spiking network
