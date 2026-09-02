/* ============================================================
   common.h
   Shared definitions used by every module of the Student Record
   Management System (student.c, fileio.c, auth.c, gui.c).
   ============================================================ */
#ifndef COMMON_H
#define COMMON_H

#define MAX_STUDENTS   100
#define NAME_LEN       50
#define DATA_FILE      "students.dat"

/* Hard-coded teacher credentials (Module 3 - auth).
   In a real deployment these would live in an encrypted/config
   file; kept simple here since it is outside the assignment's core scope. */
#define TEACHER_USER   "admin"
#define TEACHER_PASS   "admin123"

typedef struct {
    int   roll;
    char  name[NAME_LEN];
    float marks[3];
    float total;
    float average;
} Student;

/* ---------------- Module 1: student.c ----------------
   Core record operations. calculateTotalAverage() is the
   pointer-based function required by the assignment: it takes
   a Student* and writes total/average directly into that
   record, with NO global variables involved. */
void calculateTotalAverage(Student *s);

/* Adds newStudent into arr[] / *count.
   Return codes:
     >0  -> success, value = number of records added this session
             (demonstrates a STATIC counter, see getRecordCounter)
     -1  -> array is full (overflow)
     -2  -> duplicate roll number                                   */
int  addStudent(Student arr[], int *count, Student newStudent);

/* Linear search by roll number. Returns index, or -1 if not found. */
int  searchStudentByRoll(Student arr[], int count, int roll);

/* Sorts arr[] in place, descending by average (simple bubble sort,
   fine for the small N expected in a college class-list). */
void sortStudentsDescByAverage(Student arr[], int count);

/* Removes the record with the given roll number (linear search +
   shift-left). Returns 1 on success, 0 if roll number not found. */
int  deleteStudentByRoll(Student arr[], int *count, int roll);

/* Demonstrates the STATIC storage class for a counter that must
   persist across multiple calls without being a global variable.
   Pass increment = 1 to bump the counter, 0 to just read it. */
int  getRecordCounter(int increment);

/* ---------------- Module 2: fileio.c ---------------- */
/* Returns 1 on success, 0 on failure (disk full / permissions etc). */
int  saveStudentsToFile(Student arr[], int count, const char *filename);

/* Loads records from filename into arr/*count.
   If the file does not exist yet (first run), this is treated as
   a normal, recoverable condition: *count is set to 0 and the
   function returns 0 (caller does NOT show this as an error). */
int  loadStudentsFromFile(Student arr[], int *count, const char *filename);

/* ---------------- Module 3: auth.c ---------------- */
int  teacherLogin(const char *username, const char *password);

/* Looks the roll number up among already-loaded records.
   On success returns 1 and sets *foundIndex to that record's index. */
int  studentLogin(Student arr[], int count, int roll, int *foundIndex);

#endif /* COMMON_H */
