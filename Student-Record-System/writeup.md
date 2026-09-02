# Design Justification

## 1. Storage class for the record counter

`getRecordCounter()` in `student.c` uses a **`static` local variable**:

```c
int getRecordCounter(int increment) {
    static int counter = 0;
    if (increment) counter++;
    return counter;
}
```

A `static` local variable is initialised once and then keeps its value in between
calls for the entire lifetime of the program, while remaining scoped to the
function it is declared in. This is exactly what the counter needs: it must
accumulate across many calls to `addStudent()`, but nothing outside `student.c`
has any business reading or modifying it directly.

**What would go wrong with other storage classes:**
- **`auto`** (an ordinary local variable): it is created fresh on the stack every
  time the function is called and destroyed when the function returns, so it
  would reset to its initial value on every call — it could never accumulate a
  running total. The counter would always report 1 (or whatever `increment`
  passes), never a real cumulative count.
- **`extern` / true global variable**: it would work functionally (it does
  persist), but at the cost of encapsulation. Any file that includes the
  right declaration could read *and modify* it directly, bypassing the
  intended interface. In a multi-module project like this one, that makes it
  far easier to accidentally corrupt the counter from `gui.c` or introduce
  order-of-initialisation bugs across translation units. `static` gives the
  same persistence with none of that exposure.
- **`register`**: irrelevant here — it is only a (largely obsolete) hint to
  keep a variable in a CPU register, and it does not provide persistence
  between calls in the first place; a `register` local behaves like `auto`.

## 2. Identified runtime errors and how they are handled

Three realistic runtime errors were identified and handled in the code rather
than left to crash or corrupt data:

1. **Missing/first-run data file** — `loadStudentsFromFile()` calls `fopen()`
   in `"rb"` mode. If the file does not exist yet (very first run, or the
   file was deleted), `fopen()` returns `NULL`. Instead of dereferencing that
   NULL pointer (undefined behaviour / crash), the function detects it,
   sets `*count = 0`, and returns 0. The GUI simply starts with an empty
   table — a normal, expected first-run state, not an error dialog.

2. **Array overflow** — `MAX_STUDENTS` bounds the fixed-size array. Before
   writing a new record, `addStudent()` checks `*count >= MAX_STUDENTS` and
   returns `-1` rather than writing past the end of the array (which would be
   undefined behaviour and could silently corrupt adjacent memory). The GUI
   surfaces this as a clear "maximum record limit reached" message box.

3. **Duplicate roll number** — `addStudent()` performs a linear scan of the
   existing records before inserting; if the roll number already exists it
   returns `-2` instead of inserting a second, ambiguous record. The GUI
   reports this to the user instead of silently creating a duplicate entry
   that would corrupt later searches (`searchStudentByRoll()` would only ever
   find the first match) and sorting.

As a defence-in-depth measure, `loadStudentsFromFile()` also guards against a
**corrupted/truncated file**: if the stored count field is negative or larger
than `MAX_STUDENTS` (e.g. the file was hand-edited or writing was
interrupted), the function refuses it and resets to an empty, safe state
instead of letting `fread()` write past the end of the `arr[]` buffer.
