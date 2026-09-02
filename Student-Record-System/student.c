/* ============================================================
   student.c  --  MODULE 1: Core student-record operations
   ============================================================ */
#include <string.h>
#include "common.h"

/* ---- Pointer-based total/average calculator (no globals) ---- */
void calculateTotalAverage(Student *s)
{
    s->total   = s->marks[0] + s->marks[1] + s->marks[2];
    s->average = s->total / 3.0f;
}

/* ---- Add a new record, with duplicate & overflow protection ---- */
int addStudent(Student arr[], int *count, Student newStudent)
{
    int i;

    if (*count >= MAX_STUDENTS) {
        return -1;                       /* array overflow */
    }

    for (i = 0; i < *count; i++) {
        if (arr[i].roll == newStudent.roll) {
            return -2;                   /* duplicate roll number */
        }
    }

    calculateTotalAverage(&newStudent);  /* fill total/average via pointer func */
    arr[*count] = newStudent;
    (*count)++;

    return getRecordCounter(1);          /* bump + return static counter */
}

/* ---- Linear search by roll number ---- */
int searchStudentByRoll(Student arr[], int count, int roll)
{
    int i;
    for (i = 0; i < count; i++) {
        if (arr[i].roll == roll) {
            return i;
        }
    }
    return -1;
}

/* ---- Descending sort by average (bubble sort) ---- */
void sortStudentsDescByAverage(Student arr[], int count)
{
    int i, j;
    Student temp;

    for (i = 0; i < count - 1; i++) {
        for (j = 0; j < count - i - 1; j++) {
            if (arr[j].average < arr[j + 1].average) {
                temp        = arr[j];
                arr[j]      = arr[j + 1];
                arr[j + 1]  = temp;
            }
        }
    }
}

/* ---- Delete a record by roll number (linear search + shift) ---- */
int deleteStudentByRoll(Student arr[], int *count, int roll)
{
    int idx = searchStudentByRoll(arr, *count, roll);
    int i;

    if (idx == -1) {
        return 0;                        /* not found */
    }

    for (i = idx; i < *count - 1; i++) {
        arr[i] = arr[i + 1];
    }
    (*count)--;
    return 1;
}

/* ----------------------------------------------------------------
   STATIC counter demonstration.
   `counter` is declared `static` INSIDE the function:
     - It is initialised to 0 exactly once (not on every call).
     - It keeps its value between calls (persistent storage,
       allocated for the whole program lifetime).
     - It is invisible outside this function (internal linkage /
       block scope) so no other file can read or corrupt it
       directly -- unlike a true global variable.
   This is exactly the storage class the assignment write-up
   asks you to justify. See writeup.md for the full explanation.
   ---------------------------------------------------------------- */
int getRecordCounter(int increment)
{
    static int counter = 0;
    if (increment) {
        counter++;
    }
    return counter;
}
