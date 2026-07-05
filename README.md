# Lightweight Relational Database Engine

A custom-built, lightweight relational database engine written in C. This project was developed incrementally to understand the core internals of database management systems (DBMS), focusing on storage engines, indexing, and relational data structures.

## Project Structure and Progression

The repository is structured into iterative milestones (`pds1` through `pds4`), each increasing in difficulty and adding fundamental database concepts:

*   **`pds1`**: Core foundation for raw data serialization and basic file I/O operations.
*   **`pds2/pds3`**: Progressive implementations of data storage, introducing fixed-size records and early indexing concepts.
*   **`pds4` (Current Stable)**: A robust iteration featuring:
    *   **In-Memory Indexing**: Uses an `ndxArray` for fast `O(1)` or `O(N)` key lookups before hitting the disk.
    *   **Persistent Storage**: Separates data (`.dat` files) from index metadata (`.ndx` files), allowing indexes to be saved on close and loaded on startup.
    *   **Relational Integrity**: Implements `RelInfo` to track and enforce relationships (Primary Key / Foreign Key constraints) between tables.
    *   **Tombstone Deletion**: Employs a soft-delete mechanism (`is_deleted` flags) to manage deletions safely without requiring immediate file rewrites.
    *   **Full CRUD API**: Provides programmatic C functions to `create_table`, `store_table`, `get_table`, `update_table`, and `delete_table`.
*   **`pds5` (In Development)**: Upgrading the engine to use an **On-Disk B+ Tree** to remove memory constraints and handle massively scalable datasets.

## Technical Details (pds4)

*   **Language**: C
*   **Storage Mechanism**: Custom binary file formatting. Records are appended to a `.dat` file, while locations (file offsets) are tracked via an in-memory index that flushes to a `.ndx` file.
*   **Memory Management**: The current engine loads the primary index into memory up to a `MAX_REC` limit (10,000 records) to minimize disk reads for key lookups. Non-key searches utilize table scanning methodologies.

## Usage

This engine is designed to be integrated programmatically. You define your schema, initialize tables, and use the provided API to interact with the data.

### Example API Usage
```c
// 1. Initialize schema and tables
create_schema("my_database");
create_table("employees", sizeof(EmployeeStruct));

// 2. Open for transactions
open_schema("my_database");
open_table("employees");

// 3. Store and Retrieve
EmployeeStruct emp = {101, "Alice"};
store_table("employees", emp.id, &emp);

EmployeeStruct fetched_emp;
get_table("employees", 101, &fetched_emp);

// 4. Safely close and persist indexes
close_table("employees");
close_schema();
```

## Future Roadmap

- [ ] Implementation of On-Disk B+ Tree indexing for true scalability (`pds5`).
- [ ] Integration of a Buffer Pool Manager (Page Cache) to optimize disk I/O.
- [ ] Support for variable-length records (e.g., VARCHAR).
- [ ] SQL parsing and query execution engine.
