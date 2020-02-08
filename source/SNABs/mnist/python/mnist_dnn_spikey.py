'''
Trains a simple deep NN on the downscaled MNIST dataset.
Adapted from 
https://raw.githubusercontent.com/keras-team/keras/master/examples/mnist_mlp.py
'''


from __future__ import print_function

import keras
from keras.datasets import mnist
from keras.models import Sequential
from keras.layers import Dense, Dropout, MaxPooling2D, Flatten, AveragePooling2D
from keras.optimizers import RMSprop, SGD, Adam
from keras import regularizers

import numpy as np
batch_size = 128
num_classes = 10
epochs = 25

# the data, split between train and test sets
(x_train, y_train), (x_test, y_test) = mnist.load_data()

x_train = x_train.reshape(60000,28,28,1)
x_test = x_test.reshape(10000, 28,28,1)
x_train = x_train.astype('float32')
x_test = x_test.astype('float32')
x_train /= 255
x_test /= 255
print(x_train.shape, 'train samples')
print(x_test.shape, 'test samples')

# convert class vectors to binary class matrices
y_train = keras.utils.to_categorical(y_train, num_classes)
y_test = keras.utils.to_categorical(y_test, num_classes)

# = np.ndarray((60000,14,14))
#x_test_new = np.ndarray((10000,14,14))

#for counter, image in enumerate(x_train):
    ##new_im = np.zeros((14,14))
    #for i in range(0,28,2):
        #for j in range(0,28,2):
            #x_train_new[counter][int(i/2)][int(j/2)]  = (image[i][j] + image[i+1][j] + image[i][j+1] + image[i+1][j+1])/4.0
    ##x_train_new.append(new_im)


#for counter, image in enumerate(x_test):
    ##new_im = np.zeros((14,14))
    #for i in range(0,28,2):
        #for j in range(0,28,2):
            #x_test_new[counter][int(i/2)][int(j/2)] = (image[i][j] + 
                                                       #image[i+1][j] + 
                                                       #image[i][j+1] + 
                                                       #image[i+1][j+1])/4.0
    #x_test_new.append(new_im)
x_train_new = x_train
x_test_new = x_test

model = Sequential()
model.add(AveragePooling2D(pool_size=(3,3), input_shape=(28,28,1), data_format = "channels_last"))
model.add(Flatten())
model.add(Dense(100, activation='relu', 
                use_bias=False,
                kernel_constraint=keras.constraints.NonNeg(),kernel_regularizer=regularizers.l2(0.001)
                ))
model.add(Dense(num_classes, activation='relu', 
                use_bias=False, 
                kernel_constraint=keras.constraints.NonNeg(),kernel_regularizer=regularizers.l2(0.001)
                ))

model.summary()
model.compile(loss='categorical_hinge',
              optimizer=Adam(lr=0.001),#, momentum=0.0, nesterov=False),
              metrics=['accuracy'])

history = model.fit(x_train_new, y_train,
                    batch_size=batch_size,
                    epochs=epochs,
                    verbose=1,
                    validation_data=(x_test_new, y_test))
score = model.evaluate(x_test_new, y_test, verbose=0)
print('Test loss:', score[0])
print('Test accuracy:', score[1])


model.save_weights('dnn_spikey.h5')
json_string = model.to_json()
with open('dnn_spikey.json', 'w') as file:
    file.write(json_string)
