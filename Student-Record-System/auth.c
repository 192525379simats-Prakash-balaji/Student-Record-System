/* ============================================================
   auth.c  --  MODULE 3: Login / authentication
   ============================================================ */
#include <string.h>
#include "common.h"

/* Teacher: fixed username/password (see TEACHER_USER/TEACHER_PASS
   in common.h). Returns 1 if credentials match, else 0. */
int teacherLogin(const char *username, const char *password)
{
    return (strcmp(username, TEACHER_USER) == 0 &&
            strcmp(password, TEACHER_PASS) == 0);
}

/* Student: logs in with just their roll number. We look it up
   among the records already loaded from disk -- if it exists,
   the student is granted a VIEW-ONLY session limited to their
   own row (enforced by the GUI, using *foundIndex). */
int studentLogin(Student arr[], int count, int roll, int *foundIndex)
{
    int idx = searchStudentByRoll(arr, count, roll);
    if (idx == -1) {
        return 0;
    }
    *foundIndex = idx;
    return 1;
}
