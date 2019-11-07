#!/usr/bin/env python
import configparser
import json
import os
from urllib.parse import urlparse
import pika
from pymongo import MongoClient
from engine import training_dataframe, obtain_features, user_to_binary, model_training, save_scaling, load_scaling

# ---------------------------------------------------------------
# Some Configuration 
MONGO_URI = os.environ.get('MONGODB_URI', 'mongodb://cubeauth:cubeauth1233211@ds143070.mlab.com:43070/cubeauth')
RABBIT_URI = os.environ.get('RABBIT_URI', 'localhost')
basedir = os.path.abspath(os.path.dirname(__file__))
checkpoint_path = os.path.join(basedir, 'checkpoints')
logistic_regression_path = os.path.join(checkpoint_path, 'logistic_regression')
support_vector_classifier_path = os.path.join(checkpoint_path, 'support_vector_classifier')
random_forest_path = os.path.join(checkpoint_path, 'random_forest')
logs_path = os.path.join(basedir, 'logs')

if not os.path.exists(checkpoint_path):
    os.makedirs(checkpoint_path)
if not os.path.exists(logistic_regression_path):
    os.makedirs(logistic_regression_path)
if not os.path.exists(support_vector_classifier_path):
    os.makedirs(support_vector_classifier_path)
if not os.path.exists(random_forest_path):
    os.makedirs(random_forest_path)

# Following models supported for training
models = ['logRegr', 'svc', 'RandomForest']
# --------------------------------------------------------------

connection = pika.BlockingConnection(pika.URLParameters(RABBIT_URI))
channel = connection.channel()
channel.queue_declare(queue='trainings')

def callback(ch, method, properties, body):
    # Training of the model is launched
    df = training_dataframe(mongodb_uri=MONGO_URI)
    users = df['user_email'].unique()

    for model in models:
        print('Lanzando GridSearch de Hiperparámetros para modelo ', model)
        if model == 'logRegr':
            os.system("python gridsearch_logRegr.py")
            # Cargamos los parámetros idóneos para cada usuario en un json
            os.chdir(logs_path)
            with open('logRegr_GridSearch.txt', mode='r', encoding='utf-8') as f:
                grid_search = json.load(f)
            os.chdir(basedir)
        elif model == 'svc':
            os.system("python gridsearch_svc.py")
            # Cargamos los parámetros idóneos para cada usuario en un json
            os.chdir(logs_path)
            with open('svc_GridSearch.txt', mode='r', encoding='utf-8') as f:
                grid_search = json.load(f)
            os.chdir(basedir)
        elif model == 'RandomForest':
            os.system("python gridsearch_randomForest.py")
            os.chdir(logs_path)
            with open('randomForest_GridSearch.txt', mode='r', encoding='utf-8') as f:
                grid_search = json.load(f)
            os.chdir(basedir)
        # All the checkpoints to be stored in checkpoints path
        os.chdir(checkpoint_path)
        for user in users:
            print('Comenzando entrenamiento del algortimo ', model, ' para usuario ', user)
            # Clasificación binaria para cada usuario
            data = user_to_binary(df, user)
            # Aplicamos estandarización. Se guardará un fichero de estandarización en la carpeta checkpoints
            X_train, X_test, Y_train, Y_test = obtain_features(dataframe=data, random_state=42)
            if model != 'RandomForest':
                X_train = save_scaling(X_train)
                # Normalizamos el test dataset de acuerdo al training dataset 
                X_test = load_scaling(X_test)
            # Nos quedamos sólo con los hiperparámetros del usuario que nos interesan
            for item in grid_search:
                if item['user'] == user:
                    info = item['hyperparameters']

            # The training is launched for user
            model_training(x_train=X_train, x_test=X_test, y_train=Y_train, y_test=Y_test, user=user, model=model, info=info)
            
        os.chdir(basedir)

    # Connection to MongoDB is established
    client = MongoClient(MONGO_URI),
    # Getting a Database and parsing the name of the database from the MONGO_URI
    o = urlparse(MONGO_URI).path[1::]
    db = client[o]
    # Once training is finished, the user status that triggered training
    # is set to authenticable
    db.users.update_one(json.loads(body), {'$set': {'authenticable': True}})
    #print(" [x] Train triggered by %r" % json.loads(body)['email'])

channel.basic_consume(callback,
                      queue='trainings',
                      no_ack=True)

print(' [*] Waiting for messages. To exit press CTRL+C')
channel.start_consuming()


