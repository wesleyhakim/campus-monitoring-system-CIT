import sqlite3

# Replace these with your actual file path and table name
db_path = 'db.sqlite3'
table_name = 'CampusMonitor_dataentry'

# Connect to the database
conn = sqlite3.connect(db_path)
cursor = conn.cursor()

# Delete all rows from the table
cursor.execute(f"DELETE FROM {table_name};")

# Commit changes and close connection
conn.commit()
conn.close()

print(f"All entries deleted from table '{table_name}'.")

