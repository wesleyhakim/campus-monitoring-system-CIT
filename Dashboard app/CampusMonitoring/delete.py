import sqlite3

# Path to your SQLite database
db_path = 'CampusMonitoring/db.sqlite3'

# Connect to the SQLite database
conn = sqlite3.connect(db_path)
cursor = conn.cursor()

try:
    # Delete rows where value is NULL from DataEntry table
    cursor.execute("DELETE FROM CampusMonitor_DataEntry WHERE value IS NULL")
    
    # Commit the changes
    conn.commit()
    print("Deleted rows with NULL 'value' from DataEntry.")
    
except sqlite3.Error as e:
    print("An error occurred:", e)
finally:
    # Close the connection
    conn.close()
