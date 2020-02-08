from __future__ import print_function

from keras.datasets import mnist
import matplotlib
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
# the data, split between train and test sets
(x_train, y_train), (x_test, y_test) = mnist.load_data()

x_train = x_train.reshape(60000, 784)
x_test = x_test.reshape(10000, 784)
x_train = x_train.astype('float32')
x_test = x_test.astype('float32')
x_train /= 255
x_test /= 255
nr_images = 5
#for i in range(0,10):
    #print(x_train[i])
    #print(y_train[i])
    #plt.imshow(x_train[i].reshape((28,28)), cmap='gray')
    #plt.show()


for i in range(0,nr_images):
    print(y_train[i], ", ")
    
with open("train.txt", 'w') as file:
    file.write("{")
    for i in range(0,nr_images):
        file.write(str(y_train[i]))
        file.write(", ")
    file.write("}\n")
    
    file.write("{")
    for i in range(0,nr_images):
        file.write("{")
        for j in range(0,len(x_train[i])):
            file.write(str(x_train[i][j]))
            file.write(", ")
        file.write("}, \n")
    file.write("}\n")
        
with open("test.txt", 'w') as file:
    file.write("{")
    for i in range(0,nr_images):
        file.write(str(y_test[i]))
        file.write(", ")
    file.write("}\n")
    
    file.write("{")
    for i in range(0,nr_images):
        file.write("{")
        for j in range(0,len(x_test[i])):
            file.write(str(x_test[i][j]))
            file.write(", ")
        file.write("}, \n")
        
    file.write("}")
