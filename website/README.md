# Flask-Admin Dashboard 

# Flask Migrate
- When changing the model, run:  
  `flask db migrate -m 'comments on change'`

- Then run:  
  `flask db upgrade`  
  to update or create the table.

- Run:  
  `python seed.py`  
  to initialize the tables and insert sample data.
