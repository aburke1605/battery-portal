import os
import numpy as np
import pandas as pd
from datetime import datetime
from sklearn.model_selection import train_test_split
from xgboost import XGBClassifier
from sklearn.metrics import roc_curve
import matplotlib.pyplot as plt

from sqlalchemy import select, insert

from utils import upload_to_azure_blob

from app import create_app
from app import DB
from app.db import PredictionFeatures, ModelMetadata
from app.twin import decide_failure_within_7d


def train_model():
    esp_ids = DB.session.query(PredictionFeatures.esp_id).distinct().all()

    for esp_id in [row[0] for row in esp_ids]:
        decide_failure_within_7d(esp_id)

    dataframe = pd.read_sql(select(PredictionFeatures), DB.engine)
    y = dataframe["failure_within_7d"].to_numpy()
    X = dataframe.drop(
        ["timestamp", "esp_id", "failure_within_7d"], axis="columns"
    ).to_numpy()

    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.25)
    bst = XGBClassifier(
        n_estimators=2, max_depth=2, learning_rate=1, objective="binary:logistic"
    )
    bst.fit(X_train, y_train)
    pred_proba = bst.predict_proba(X_test)
    fpr, tpr, *_ = roc_curve(y_test, pred_proba[:, 1])

    RUNNING_IN_AZURE = "WEBSITE_INSTANCE_ID" in os.environ
    if RUNNING_IN_AZURE:
        path = "/tmp"
    else:
        path = "."

    model_name = "xgb"
    new_version = 0
    try:
        creation_timestamp = datetime.now()
        active_models = DB.session.execute(
            ModelMetadata.query.filter_by(active=True)
        ).fetchall()
        n_active_models = len(active_models)
        if n_active_models >= 1:
            ModelMetadata.query.update({ModelMetadata.active: False})
            DB.session.commit()
        versions = np.sort(
            np.ravel(
                np.array(
                    DB.session.query(ModelMetadata.version)
                    .where(ModelMetadata.model_name == model_name)
                    .all()
                )
            )
        )
        new_version = int(versions[-1] + 1) if len(versions) > 0 else 1
        DB.session.execute(
            insert(ModelMetadata).values(
                model_name=model_name,
                version=new_version,
                storage_uri=f"https://batteryportalstorage.blob.core.windows.net/xgb-models/{model_name}/{new_version}/model.json",
                creation_timestamp=creation_timestamp,
                active=True,
            )
        )
        DB.session.commit()
    except Exception as e:
        print("Could not update model_metadata:", e)

    fig, ax = plt.subplots()
    ax.plot([0, 1], [0, 1], linestyle=":")
    ax.scatter(fpr, tpr)
    fig.savefig(f"{path}/ROC.pdf")
    bst.save_model(f"{path}/{model_name}_{new_version}.json")

    if path == "/tmp":
        upload_to_azure_blob(
            f"{path}/ROC.pdf", "xgb-models", f"{model_name}/{new_version}/ROC.pdf"
        )
        upload_to_azure_blob(
            f"{path}/{model_name}_{new_version}.json",
            "xgb-models",
            f"{model_name}/{new_version}/model.json",
        )


if __name__ == "__main__":
    app = create_app()
    with app.app_context():
        train_model()
