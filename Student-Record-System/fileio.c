/* ============================================================
   fileio.c  --  MODULE 2: Persistent storage (binary file I/O)
   ============================================================ */
#include <stdio.h>
#include "common.h"

/* ---- Save all records to disk (binary format: count + array) ---- */
int saveStudentsToFile(Student arr[], int count, const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        return 0;                                /* couldn't open for writing */
    }

    fwrite(&count, sizeof(int), 1, fp);
    fwrite(arr, sizeof(Student), (size_t)count, fp);

    fclose(fp);
    return 1;
}

/* ----------------------------------------------------------------
   Load records from disk.

   Runtime-error handling implemented here (the assignment's
   "identify + fix a runtime error" requirement):

   1) MISSING FILE (first run, or file deleted/renamed):
      fopen() returns NULL. Instead of crashing or printing a
      scary error, we treat this as "no records yet": *count = 0,
      return 0. The GUI simply shows an empty table -- graceful,
      not fatal.

   2) CORRUPTED / TRUNCATED FILE:
      If the stored count is unreasonable (negative, or bigger
      than MAX_STUDENTS -- e.g. the file was edited by hand or
      cut off mid-write), we clamp it to a safe range instead of
      letting fread() overflow the fixed-size arr[] buffer.
   ---------------------------------------------------------------- */
int loadStudentsFromFile(Student arr[], int *count, const char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        *count = 0;
        return 0;                                /* no file yet -- not an error */
    }

    if (fread(count, sizeof(int), 1, fp) != 1) {
        *count = 0;                              /* empty/corrupt file */
        fclose(fp);
        return 0;
    }

    if (*count < 0 || *count > MAX_STUDENTS) {
        *count = 0;                              /* corrupted count -- refuse it */
        fclose(fp);
        return 0;
    }

    fread(arr, sizeof(Student), (size_t)(*count), fp);

    fclose(fp);
    return 1;
}
