# Student Record Management System (C, Win32 GUI)

A modular C application with a native Windows GUI (Win32 API — no external
GUI library required, just `windows.h` which ships with MinGW). Built for
VS Code + MinGW-w64.

## Modules

| File | Module | Responsibility |
|---|---|---|
| `common.h` | shared | `Student` struct, constants, all prototypes |
| `student.c` | 1 — Core logic | pointer-based total/average calculator, linear search, descending sort by average, `static`-counter demo |
| `fileio.c` | 2 — Persistence | binary save/load, missing-file & corrupt-file handling |
| `auth.c` | 3 — Auth | teacher login (username/password), student login (roll number) |
| `gui.c` | 4 — GUI | Win32 login window, teacher dashboard (ListView + Add/Search/Sort/Save/Logout), student dashboard (read-only, own record) |

## Features

- **Login screen** with role selector (Teacher / Student) — mandatory, shown first.
- **Teacher**: sees a table (ListView) of every record; can **Add** a student
  (with duplicate-roll and overflow checks), **Search by roll number**,
  **Sort** the table descending by average, **Save** to disk, **Logout**.
- **Student**: logs in with just their roll number and sees a **view-only**
  screen of *only their own* record (name, 3 subject marks, total, average).
- **Persistence**: records are stored in `students.dat` next to the .exe
  (binary format). Data survives closing and reopening the app — loaded on
  startup, saved on every Add / Save-to-File / Logout / window-close.

## Build (VS Code + MinGW64)

1. Install MinGW-w64 and make sure `gcc` is on your PATH (`gcc --version`
   in a terminal should work).
2. Open this folder in VS Code.
3. Either:
   - Press **Ctrl+Shift+B** (uses `.vscode/tasks.json`), or
   - Run `build.bat` from the integrated terminal, or
   - Run manually:
     ```
     gcc gui.c student.c fileio.c auth.c -o StudentRecordSystem.exe -lcomctl32 -mwindows
     ```
4. Run `StudentRecordSystem.exe`.

## Default login

- **Teacher** — username: `admin`, password: `admin123`
  (change `TEACHER_USER`/`TEACHER_PASS` in `common.h` if you want different
  credentials).
- **Student** — enter any roll number that already exists in `students.dat`.
  Add a student as the teacher first, then log out and log back in as that
  student to see the view-only screen.

## Design notes

See `writeup.md` for the required half-page justification of:
1. Why the record counter uses the **`static`** storage class, and what
   would break with `auto` or a true global instead.
2. The runtime errors identified (missing file, array overflow, duplicate
   roll number, corrupted file) and how each is handled gracefully instead
   of crashing or corrupting data.

## Notes / possible extensions (not required, easy to bolt on if you want more marks)

- Delete / Edit record buttons (straightforward — same popup-window pattern as Add).
- Per-teacher accounts stored in a `teachers.dat` file instead of one hard-coded login.
- CSV export button next to "Save to File" for opening records in Excel.
