from pathlib import Path
import progressbar
import time
from utils import upload_to_azure_blob

directory = Path("../simulation/data/")

for path in directory.iterdir():
    simulation_type = path.name
    files = list(path.iterdir())
    print(f"uploading {simulation_type}...")
    for file in progressbar.progressbar(files, redirect_stdout=True):
        if file.suffix == ".csv":
            upload_to_azure_blob(
                file.as_posix(),
                "example-data",
                f"simulated_data/{simulation_type}/{file.name}",
            )
