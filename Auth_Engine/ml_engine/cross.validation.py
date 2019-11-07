import json
import os
import warnings
import numpy as np
import random
import pickle
import sys
from sklearn.metrics import precision_recall_fscore_support
from sklearn.exceptions import DataConversionWarning
warnings.filterwarnings(action='ignore', category=DataConversionWarning)
warnings.filterwarnings(action='ignore', category=FutureWarning)
from engine import training_dataframe, obtain_features, user_to_binary, model_training, save_scaling, load_scaling

# Some Configuration 
MONGO_URI = os.environ.get('MONGODB_URI', 
                           'mongodb://cubeauth:cubeauth1233211@ds143070.mlab.com:43070/cubeauth')
basedir = os.path.abspath(os.path.dirname(__file__))
checkpoint_path = os.path.join(basedir, 'checkpoints')
logistic_regression_path = os.path.join(checkpoint_path, 'logistic_regression')
support_vector_classifier_path = os.path.join(checkpoint_path, 'support_vector_classifier')
random_forest_path = os.path.join(checkpoint_path, 'random_forest')
logs_path = os.path.join(basedir, 'logs')

# Following models to be supported
model = 'svc' #['logRegr', 'svc', 'RandomForest']

# Loading dataframe from database
df = training_dataframe(mongodb_uri=MONGO_URI)
# Users involved in the experiment so far
users = df['user_email'].unique()

n_loops = 5

os.chdir(checkpoint_path)

for user in users:

    p = list() 
    r = list()
    f1 = list()

    data = user_to_binary(df, user)
        
    if np.where(data.user==1)[0].__len__() >= 10:

        for _ in range(n_loops):
            
            try:
                # Realizamos la partición del dataset
                X_train, X_test, Y_train, Y_test = obtain_features(dataframe=data, random_state=random.randint(1,100))
            except ValueError:
                # Realizamos la partición del dataset
                X_train, X_test, Y_train, Y_test = obtain_features(dataframe=data, random_state=random.randint(1,100))
            
            if model != 'RandomForest':
                # Normalizamos el test dataset de acuerdo al training 
                # # dataset sobre el que se ha hecho oversampling
                X_test = load_scaling(X_test)
            
            if model == 'logRegr':
                filename = 'logistic_regression/' + user +'.sav'
            elif model == 'svc':
                filename =  'support_vector_classifier/' + user +'.sav'
            elif model == 'RandomForest':
                filename =  'random_forest/' + user +'.sav'
            else:
                print('Specify a correct model keyword')
                sys.exit()
    
            loaded_model = pickle.load(open(filename, 'rb'))
            
            y_pred = loaded_model.predict(X_test)
            y_true = np.array(Y_test)
            metrics = precision_recall_fscore_support(y_true, y_pred, average=None, labels=[1, 0])

            p.append(np.round(metrics[0][0], decimals=2))
            r.append(np.round(metrics[1][0], decimals=2))
            f1.append(np.round(metrics[2][0], decimals=2))

        info = {} 
        info['user'] = user
        info['p'] = np.mean(p)
        info['r'] = np.mean(r)
        info['f1'] = np.mean(f1)
        info['p_err'] = 2.13*np.std(p)/np.sqrt(n_loops)
        info['r_err'] = 2.13*np.std(r)/np.sqrt(n_loops)
        info['f1_err'] = 2.13*np.std(f1)/np.sqrt(n_loops)

    print(info)

os.chdir(basedir)


