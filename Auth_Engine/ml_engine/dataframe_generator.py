import json
import os
import warnings
import numpy as np
from sklearn.exceptions import DataConversionWarning
warnings.filterwarnings(action='ignore', category=DataConversionWarning)
warnings.filterwarnings(action='ignore', category=FutureWarning)
from engine import training_dataframe, user_to_binary, obtain_features
import pandas as pd

# Some Configuration 
MONGO_URI = os.environ.get('MONGODB_URI', 
                           'mongodb://cubeauth:cubeauth1233211@ds143070.mlab.com:43070/cubeauth')

# Loading dataframe from database
df = training_dataframe(mongodb_uri=MONGO_URI)
# Hay que resolver algunos problemas a la hora de discretizar usuarios
df['user_email'].replace([
                          'victormanuel.mundillagarcia@11paths.com', 
                          'victormanuel.mundillagarcia@telefonica.com', 
                          'victormanuel.mundilla@11paths.com'
                          ]
                          ,
                          [
                           'victor.mundilla@11paths.com',
                           'victor.mundilla@11paths.com',
                           'victor.mundilla@11paths.com'
                          ], 
                          inplace=True)

df['user_email'].replace([
                          'arniawan.soekarno.ext@telefonica.com'
                          ]
                          ,
                          [
                           'arniawan.soekarno@telefonica.com'
                          ], 
                          inplace=True)

df_onehot = pd.concat([df,pd.get_dummies(df['user_email'], prefix='user_email')], axis=1)
df_onehot.drop(['user_email'],axis=1, inplace=True)
df_onehot.to_csv('dataframe_onehot.csv')


users = df['user_email'].unique()
print(users)


for user in users:

    data = user_to_binary(df, user=user)

    X_train, X_test, Y_train, Y_test = obtain_features(dataframe=data, random_state=42)


    feat = pd.concat([X_train, X_test], ignore_index=True)
    labels = pd.concat([Y_train, Y_test], ignore_index=True)

    dataframe = pd.concat([feat, labels], axis=1)

    dataframe.to_csv('cubeauth_{}.csv'.format(user))


