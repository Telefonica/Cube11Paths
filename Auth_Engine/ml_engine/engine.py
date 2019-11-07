# usr/bin/env python
import configparser
import itertools
import os
from urllib.parse import urlparse
import numpy as np
from pymongo import MongoClient
import sys
import pandas as pd
from sklearn import model_selection
from sklearn.metrics import precision_recall_fscore_support
import pickle
import json
import numpy as np
from sklearn.metrics import confusion_matrix


def model_training(x_train, x_test, y_train, y_test, user, model, info):
    """
    Rutina de entrenamiento
    Los datos de entrada deben estar normalizados (salvo en RandomForest)
    Los hiperparámetros son exclusivos de cada usuario
    Se pasan en el parámetro info como un json y se acceden 
    en función del modelo al que estemos atacando
    """

    if model == 'logRegr':
        from sklearn.linear_model import LogisticRegression

        # Información única para cada usuario
        C = info['C']
        class_weight = info['class_weight']
        penalty = info['penalty']
        solver = info['solver']

        model = LogisticRegression(C=C, 
                                   class_weight=class_weight, 
                                   solver=solver, 
                                   max_iter=1000, 
                                   penalty=penalty, 
                                   random_state=42)
        model.fit(x_train, y_train)
        
        y_pred = model.predict(x_test)

        filename = 'logistic_regression/' + user +'.sav'

    elif model == 'svc':
        from sklearn.svm import SVC

        # Información única para cada usuario
        C = info['C']
        kernel = info['kernel']
        if kernel == 'linear':
            model = SVC(C=C, kernel=kernel, probability=True, random_state=42)
        elif kernel == 'rbf':
            gamma = info['gamma']
            model = SVC(C=C, gamma=gamma, kernel=kernel, probability=True, random_state=42)
            
        model.fit(x_train, y_train)
        
        y_pred = model.predict(x_test)

        filename = 'support_vector_classifier/' + user +'.sav'
    
    elif model == 'RandomForest':
        from sklearn.ensemble import RandomForestClassifier

        # Información única para cada usuario
        max_features = info['max_features']
        max_depth = info['max_depth']
        min_samples_leaf = info['min_samples_leaf']
        min_samples_split = info['min_samples_split']
        n_estimators = info['n_estimators']
        criterion = info['criterion']

        model = RandomForestClassifier(
            n_estimators=n_estimators, 
            criterion=criterion, 
            max_depth=max_depth, 
            min_samples_split=min_samples_split, 
            min_samples_leaf=min_samples_leaf, 
            min_weight_fraction_leaf=0.0, 
            max_features=max_features, 
            max_leaf_nodes=None, 
            min_impurity_decrease=0.0, 
            min_impurity_split=None, 
            bootstrap=True, 
            oob_score=False, 
            n_jobs=None, 
            random_state=42, 
            verbose=0, 
            warm_start=False, 
            class_weight=None
        )

        model.fit(x_train, y_train)

        y_pred = model.predict(x_test)

        filename = 'random_forest/' + user +'.sav'
    
    y_true = np.array(y_test)
    metrics = precision_recall_fscore_support(y_true, y_pred, average=None, labels=[1, 0])

    # Confusion matrix
    _conf = confusion_matrix(y_true, y_pred).ravel()

    # Contamos el número real de positivos y negativos
    pos = np.count_nonzero(y_true)
    neg = y_true.__len__() - pos

    # Guardamos checkpoint
    pickle.dump(model, open(filename, 'wb'))

    print('Precision: ', np.round(metrics[0][0], decimals=2), 
    ' ||', 'Recall: ', np.round(metrics[1][0], decimals=2), 
    ' ||', 'F-Score: ', np.round(metrics[2][0], decimals=2))
    print('tn: ', _conf[0], 
    ' || fp: ', _conf[1], 
    ' || fn: ', _conf[2],  
    ' || tp: ', _conf[3], 
    ' || pos: ', pos, 
    ' || neg:', neg)
    return metrics[0][0], metrics[1][0], metrics[2][0]


def model_testing(testeo, user, model):
    """
    Rutina de testeo.
    Los datos se normalizan dentro de la función
    """

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
    if model != 'RandomForest':
        # Cargamos el escalado desde el fichero scaler.sav y 
        # escalamos la secuencia de testeo
        testeo = load_scaling(testeo)
    probs = list()
    probs.append(loaded_model.predict_proba(testeo)[0][1])
    # Vamos a meter algo de ruido sobre la secuencia de testeo 
    # ya normalizada (o no, si es RandomForest)
    for _ in range(100):
        signal = testeo + np.random.uniform(0.0, 1.5, testeo.shape)
        probs.append(loaded_model.predict_proba(signal)[0][1])
    
    # probability = loaded_model.predict_proba(testeo)[0][1]
    probability = np.median(probs)
    # Devolvemos la probabilidad de que sea un 1
    return probability


def training_dataframe(mongodb_uri): 

    # Making a Connection with MongoClient
    client = MongoClient(mongodb_uri)
    o = urlparse(mongodb_uri).path[1::]
    # Getting a Database and parsing the name of the database from the MongoURI
    db = client[o]
    # Querying in MongoDB and obtaining the result as variable
    movements_collection = list(db.movements.find())

    movements = pd.DataFrame(movements_collection)

    gb_sequence = movements.groupby('sequence')
    # Split DataFrame grouped by sequences
    sequences = [gb_sequence.get_group(x) for x in gb_sequence.groups]

    info_bucket = list()
    # Recorremos cada uno de los dataframes en sequences
    for df in sequences:

        _id = pd.unique(df['_id'])[0]
        cube_type = pd.unique(df['cube_type'])[0]
        is_random = pd.unique(df['is_random'])[0]
        user = pd.unique(df['user_email'])[0]
        time = (df['timestamp'] - df['timestamp'].iloc[0]) / np.timedelta64(1, 's')

        num_moves = len(time)
        duration = time.iloc[-1]
        # Las diferencias de tiempo
        diff_time = np.diff(time)
        # Handling positioning
        yaw_pitch_roll = df['yaw_pitch_roll']
        if yaw_pitch_roll.iloc[0]:
            y = list(); p = list(); r = list()
            y_sigma = list(); p_sigma = list(); r_sigma = list()
            for bulk in yaw_pitch_roll:
                x = list(); y = list(); z = list()
                for item in bulk:
                    x.append(item['x']); y.append(item['y']); z.append(item['z'])
                    yaw = np.mean(x); pitch = np.mean(y); roll = np.mean(z)
                    yaw_std = np.std(x); pitch_std = np.std(y); roll_std = np.std(z)
                y.append(yaw); p.append(pitch); r.append(roll)
                y_sigma.append(yaw_std); p_sigma.append(pitch_std); r_sigma.append(roll_std)
        else:
            y = [0] * df.shape[0]
            p = [0] * df.shape[0]
            r = [0] * df.shape[0]
            y_sigma = [0] * df.shape[0]
            p_sigma = [0] * df.shape[0]
            r_sigma = [0] * df.shape[0]
        
        YAW = np.std(y); PITCH = np.std(p); ROLL = np.std(r)
        YAW_SIGMA = np.mean(y_sigma); PITCH_SIGMA = np.mean(p_sigma); ROLL_SIGMA = np.mean(r_sigma)
        
        try:
            # Nos interesan mínimo, máximo, Mediana, 25th percentile and 75th percentile
            q1 = np.percentile(diff_time, 25)
            q2 = np.percentile(diff_time, 50)
            q3 = np.percentile(diff_time, 75)
        except IndexError:
            q1 = q2 = q3 = 0
        iqr = (q3 - q1)/np.sqrt(num_moves)
        maximum = q3 + 1.58 * iqr
        minimum = q1 - 1.58 * iqr
        
        turn_values = ['11', '13', '21', '23', 
                       '31', '33', '41', '43', 
                       '51', '53', '61', '63']
        chk = df.groupby('turn').size()
        chk_index = chk.index

        for i in turn_values:
            if i not in chk_index:
                chk.loc[i] = 0
        
        no_11 = chk.loc['11']
        no_13 = chk.loc['13']
        no_21 = chk.loc['21']
        no_23 = chk.loc['23']
        no_31 = chk.loc['31']
        no_33 = chk.loc['33']
        no_41 = chk.loc['41']
        no_43 = chk.loc['43']
        no_51 = chk.loc['51']
        no_53 = chk.loc['53']
        no_61 = chk.loc['61']
        no_63 = chk.loc['63']

        # no_CW = no_11 + no_21 + no_31 + no_41 + no_51 + no_61
        # no_CCW = no_13 + no_23 + no_33 + no_43 + no_53 + no_63

        # Definimos un diccionario vacío
        df_dict = {}

        df_dict['user_email'] = user
        df_dict['cube_type'] = cube_type
        df_dict['is_random'] = is_random
        df_dict['num_moves'] = num_moves
        df_dict['duration'] = duration
        # df_dict['q1'] = q1
        df_dict['q2'] = q2
        # df_dict['q3'] = q3
        # df_dict['iqr'] = iqr
        # df_dict['maximum'] = maximum
        # df_dict['minimum'] = minimum
        df_dict['no_11'] = no_11 / num_moves
        df_dict['no_13'] = no_13 / num_moves
        df_dict['no_21'] = no_21 / num_moves
        df_dict['no_23'] = no_23 / num_moves
        df_dict['no_31'] = no_31 / num_moves
        df_dict['no_33'] = no_33 / num_moves
        df_dict['no_41'] = no_41 / num_moves
        df_dict['no_43'] = no_43 / num_moves
        df_dict['no_51'] = no_51 / num_moves
        df_dict['no_53'] = no_53 / num_moves
        df_dict['no_61'] = no_61 / num_moves
        df_dict['no_63'] = no_63 / num_moves
        # df_dict['no_CW'] = no_CW / num_moves
        # df_dict['no_CCW'] = no_CCW / num_moves
        # Handling positioning
        df_dict['yaw'] = YAW
        df_dict['pitch'] = PITCH
        df_dict['roll'] = ROLL
        df_dict['yaw-sigma'] = YAW_SIGMA
        df_dict['pitch-sigma'] = PITCH_SIGMA
        df_dict['roll-sigma'] = ROLL_SIGMA

        info_bucket.append(df_dict)

    dataframe = pd.DataFrame(info_bucket)
    dataframe = dataframe.fillna(value=0)
    return dataframe


def testing_dataframe(body):

    user = json.loads(body.decode('utf-8'))['email']
    login_id = json.loads(body.decode('utf-8'))['id']
    movements = json.loads(body.decode('utf-8'))['movements']
    
    cube_type = movements[0]['cube_type']
    is_random = movements[0]['is_random']

    # Pasamos todos los movimientos a un pandas DataFrame

    movements = pd.DataFrame(movements)
    movements = movements.convert_objects(convert_numeric=True)
    movements.turn = movements.turn.astype(str)
    movements['timestamp'] = pd.to_datetime(movements['timestamp'], infer_datetime_format=True)
    time = (movements['timestamp'] - movements['timestamp'].iloc[0]) / np.timedelta64(1, 's')
    num_moves = len(time)
    duration = time.iloc[-1]
    # Las diferencias de tiempo
    diff_time = np.diff(time)

    # Handling positioning
    yaw_pitch_roll = movements['yaw_pitch_roll']
    if yaw_pitch_roll.iloc[0]:
        y = list(); p = list(); r = list()
        y_sigma = list(); p_sigma = list(); r_sigma = list()
        for bulk in yaw_pitch_roll:
            x = list(); y = list(); z = list()
            for item in bulk:
                x.append(item['x']); y.append(item['y']); z.append(item['z'])
                yaw = np.mean(x); pitch = np.mean(y); roll = np.mean(z)
                yaw_std = np.std(x); pitch_std = np.std(y); roll_std = np.std(z);
            y.append(yaw); p.append(pitch); r.append(roll)
            y_sigma.append(yaw_std); p_sigma.append(pitch_std); r_sigma.append(roll_std)
    else:
        y = [0] * movements.shape[0]
        p = [0] * movements.shape[0]
        r = [0] * movements.shape[0]
        y_sigma = [0] * movements.shape[0]
        p_sigma = [0] * movements.shape[0]
        r_sigma = [0] * movements.shape[0]
    
    YAW = np.std(y); PITCH = np.std(p); ROLL = np.std(r)
    YAW_SIGMA = np.mean(y_sigma); PITCH_SIGMA = np.mean(p_sigma); ROLL_SIGMA = np.mean(r_sigma)

    
    info_bucket = list()

    try:
        # Nos interesan mínimo, máximo, Mediana, 25th percentile and 75th percentile
        q1 = np.percentile(diff_time, 25)
        q2 = np.percentile(diff_time, 50)
        q3 = np.percentile(diff_time, 75)
    except IndexError:
        q1 = q2 = q3 = 0
    iqr = (q3 - q1)/np.sqrt(num_moves)
    maximum = q3 + 1.58 * iqr
    minimum = q1 - 1.58 * iqr
    
    turn_values = ['11', '13', '21', '23', 
                '31', '33', '41', '43', 
                '51', '53', '61', '63']
    chk = pd.DataFrame([movements.groupby('turn').size()])
    chk_index = chk.keys().format()

    for i in turn_values:
        if i not in chk_index:
            # chk.loc[i] = 0
            chk[str(i)] = 0
    
    no_11 = int(chk['11'].values)
    no_13 = int(chk['13'].values)
    no_21 = int(chk['21'].values)
    no_23 = int(chk['23'].values)
    no_31 = int(chk['31'].values)
    no_33 = int(chk['33'].values)
    no_41 = int(chk['41'].values)
    no_43 = int(chk['43'].values)
    no_51 = int(chk['51'].values)
    no_53 = int(chk['53'].values)
    no_61 = int(chk['61'].values)
    no_63 = int(chk['63'].values)

    #no_CW = no_11 + no_21 + no_31 + no_41 + no_51 + no_61
    #no_CCW = no_13 + no_23 + no_33 + no_43 + no_53 + no_63

    # Definimos un diccionario vacío
    df_dict = {}

    df_dict['user_email'] = user
    df_dict['cube_type'] = cube_type
    df_dict['is_random'] = is_random
    df_dict['num_moves'] = num_moves
    df_dict['duration'] = duration
    # df_dict['q1'] = q1
    df_dict['q2'] = q2
    # df_dict['q3'] = q3
    # df_dict['iqr'] = iqr
    # df_dict['maximum'] = maximum
    # df_dict['minimum'] = minimum
    df_dict['no_11'] = no_11 / num_moves
    df_dict['no_13'] = no_13 / num_moves
    df_dict['no_21'] = no_21 / num_moves
    df_dict['no_23'] = no_23 / num_moves
    df_dict['no_31'] = no_31 / num_moves
    df_dict['no_33'] = no_33 / num_moves
    df_dict['no_41'] = no_41 / num_moves
    df_dict['no_43'] = no_43 / num_moves
    df_dict['no_51'] = no_51 / num_moves
    df_dict['no_53'] = no_53 / num_moves
    df_dict['no_61'] = no_61 / num_moves
    df_dict['no_63'] = no_63 / num_moves
    #df_dict['no_CW'] = no_CW / num_moves
    #df_dict['no_CCW'] = no_CCW / num_moves
    # Handling positioning
    # df_dict['yaw'] = YAW
    # df_dict['pitch'] = PITCH
    # df_dict['roll'] = ROLL
    # df_dict['yaw-sigma'] = YAW_SIGMA
    # df_dict['pitch-sigma'] = PITCH_SIGMA
    # df_dict['roll-sigma'] = ROLL_SIGMA

    info_bucket.append(df_dict)
    dataframe = pd.DataFrame(info_bucket)
    dataframe = dataframe.fillna(value=0)
    dataframe = dataframe.drop(['user_email', 'cube_type', 'is_random'], axis=1)
    return user, dataframe, login_id


def user_to_binary(dataframe, user):
    transform_user = dataframe['user_email']==user
    dataframe['user'] = transform_user.astype('int')
    return dataframe


def obtain_features(dataframe, random_state, test_size=0.3):
    """
    Esta función coge un dataframe en crudo desde la base de datos, 
    se carga las columnas que no nos interesan y nos devuelve una 
    partición en train y test dataset.
    Se procede a un SMOTE oversampling con la intención de compensar las 
    pocas muestras que tenemos en el dataset. Es mejor esto que simplemente copiar
    las muestras de la clase subrepresentada, lo cual está soportado en la función 
    smote().
    """
    
    X = dataframe.drop(['user', 'user_email', 'cube_type', 'is_random'], axis=1)
    Y = dataframe['user']

    # Hacemos una partición del dataset en entrenamiento y testeo

    X_train, X_test, Y_train, Y_test = model_selection.train_test_split(X, Y, 
                                                                        test_size=test_size, 
                                                                        random_state=random_state)

    # Primero hacemos un upsampling parcial a una tasa de resampling_factor
    # tanto del training como del test dataset. Así evitamos errores antes de 
    # hacer SMOTE sobre estas muestras

    train_upsampled = upsample(pd.concat([X_train, Y_train], axis=1))
    test_upsampled = upsample(pd.concat([X_test, Y_test], axis=1))

    # Tenemos pocas muestras de dataset, y además desbalanceadas. 
    # Por ello, debemos intentar hacer data augmentation. 
    # Usamos SMOTE.

    train = smote(train_upsampled); test = smote(test_upsampled)
    X_train, Y_train = train.drop(['user'], axis=1), train['user']
    X_test, Y_test = test.drop(['user'], axis=1), test['user']

    return X_train, X_test, Y_train, Y_test


def save_scaling(dataframe, scalerfile='scaler.sav'):
    from sklearn.preprocessing import StandardScaler

    # The StandardScaler model is stored for further testing
    scaler = StandardScaler()

    scaling = scaler.fit(dataframe.values)

    pickle.dump(scaling, open(scalerfile, 'wb'))
    # Transformamos el dataframe entre -1 y 1
    dataframe = pd.DataFrame(scaler.fit_transform(dataframe), columns=dataframe.keys())
    # Ya lo tenemos escalado
    return dataframe


def load_scaling(dataframe, scalerfile='scaler.sav'):
    """
    Function devoted to testing purposes after FrontEnd auth attempt
    """
    scaler = pickle.load(open(scalerfile, 'rb'))

    # Hacemos una transformación de los datos en base al escalado ya guardado
    dataframe = pd.DataFrame(scaler.transform(dataframe), columns=dataframe.keys())

    return dataframe


def smote(df):
    """
    Aplicamos SMOTE sobre un dataset desbalanceado
    """
    from imblearn.over_sampling import SMOTE # SMOTE: Synthetic Minority Over-sampling Technique
    k_neighbors = 5

    sm = SMOTE(sampling_strategy='minority', k_neighbors=k_neighbors)#, random_state=42)
    x_oversampled, y_oversampled = sm.fit_sample(df.drop(['user'], axis=1), df['user'])
    
    X_oversampled = pd.DataFrame(x_oversampled, columns=df.drop(['user'], axis=1).keys())
    Y_oversampled = pd.DataFrame(y_oversampled, columns={'user'})
    df = pd.concat([X_oversampled, Y_oversampled], axis=1)
    return df


def upsample(df):
    from sklearn.utils import resample

    # Separate majority and minority classes
    df_majority = df[df.user==0]
    df_minority = df[df.user==1]

    # Resampling factor
    resampling_factor = 0.5
    
    # Upsample minority class
    df_minority_upsampled = resample(df_minority, 
                                    replace=True,     # sample with replacement
                                    # to match majority class
                                    n_samples=int(df_majority.__len__() * resampling_factor),    
                                    random_state=42) # reproducible results
    
    # Combine majority class with upsampled minority class
    df_upsampled = pd.concat([df_majority, df_minority_upsampled])

    return df_upsampled


def downsample(df):
    from sklearn.utils import resample

    # Separate majority and minority classes
    df_majority = df[df.user==0]
    df_minority = df[df.user==1]
    
    # Downsample majority class
    df_majority_downsampled = resample(df_majority, 
                                    replace=False,    # sample without replacement
                                    n_samples=df_minority.__len__(),     # to match minority class
                                    random_state=42) # reproducible results
    
    # Combine minority class with downsampled majority class
    df_downsampled = pd.concat([df_majority_downsampled, df_minority])

    return df_downsampled

