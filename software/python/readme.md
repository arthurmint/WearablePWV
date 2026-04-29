How to run the software inside this folder:

1) Ensure you have Python installed

2) Create a virtual environment

```python
python -m venv ./venv
```

3) Activate your virtual environment

```python
source my_project-venv/bin/activate
```

4) Install dependencies

```python
cd venv
pip install -r requirements.txt
```

5) Run Python scripts from within venv
```python
python ../serialLogger.py
python ../serialPlotter.py
```
