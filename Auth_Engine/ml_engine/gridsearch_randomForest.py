# usr/bin/env python

# https://towardsdatascience.com/predicting-stars-galaxies-quasars-with-random-forest-classifiers-in-python-edb127878e43
# https://chrisalbon.com/machine_learning/model_evaluation/cross_validation_parameter_tuning_grid_search/

import json
import os
import numpy as np
from engine import training_dataframe, obtain_features, user_to_binary, save_scaling, load_scaling

# Some configuration
basedir = os.path.abspath(os.path.dirname(__file__))
MONGO_URI = os.environ.get('MONGODB_URI', 
                           'mongodb://cubeauth:cubeauth1233211@ds143070.mlab.com:43070/cubeauth')
checkpoint_path = os.path.join(basedir, 'checkpoints')
logs_path = os.path.join(basedir, 'logs')

def main():
  df = training_dataframe(mongodb_uri=MONGO_URI)
  users = df['user_email'].unique()

  data = list()

  os.chdir(checkpoint_path)
  for user in users:
      # All the checkpoints to be stored in checkpoints path
          
      df = user_to_binary(df, user)

      if np.where(df.user==1)[0].__len__() >= 10:
      
        X_train, X_test, Y_train, Y_test = obtain_features(dataframe=df, random_state=42)

        from sklearn.ensemble import RandomForestClassifier
        from sklearn.model_selection import RandomizedSearchCV
        model = RandomForestClassifier()

        from pprint import pprint
        pprint(model.get_params())

        hyperparameters = {'max_features':[None, 'auto', 'sqrt', 'log2'],
                          'max_depth':[None, 1, 5, 10, 15, 20],
                          'min_samples_leaf': [1, 2, 4, 6],
                          'min_samples_split': [2, 4, 6, 8, 10],
                          'n_estimators': [int(x) for x in np.linspace(start = 10, stop = 100, num = 10)],
                          'criterion': ['gini', 'entropy']}

        rf_random = RandomizedSearchCV(model, hyperparameters, 
                                      n_iter = 100, 
                                      cv = 5, 
                                      verbose=2, 
                                      random_state=42, 
                                      n_jobs = -1)
        
        # Train the classifier on X_train and Y_train
        rf_random.fit(X_train, Y_train)
        # A huge bunch of stuff comes up. To obtain the best parameters, we call:
        pprint(rf_random.best_params_)

        # Almacenamos la información
        info = {}
        info['user'] = user
        info['hyperparameters'] = rf_random.best_params_
        data.append(info)

        print('Best score for training_data:', rf_random.best_score_)
  os.chdir(basedir)

  os.chdir(logs_path)
  with open("randomForest_GridSearch.txt", "w") as myfile:
    json.dump(data, myfile)
  os.chdir(basedir)

if __name__ == '__main__':
    main()