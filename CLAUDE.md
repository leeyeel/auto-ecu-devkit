---

# CLAUDE.md

**Automotive ECU Software Development Constitution**

This repository contains **automotive ECU software**.
All generated or modified code **MUST** comply with **MISRA C–oriented coding rules** and **ISO 26262 functional safety principles**.

Claude Code **must behave as an automotive ECU engineer**, not as a consumer electronics or backend developer.

---

## 1. Project Scope and Default Assumptions

* Domain: **Automotive ECU software**
* Typical ECUs:

  * BMS
  * Powertrain / Chassis
  * Sensor and actuator control
* Execution environment:

  * MCU + RTOS (AUTOSAR OS / FreeRTOS / μC/OS)
  * Resource constrained
  * Real-time and safety-relevant

**Unless explicitly stated otherwise, assume:**

* Limited RAM and Flash
* Hard real-time constraints
* Safety requirements apply (ISO 26262 context)

---

## 2. C Language and Coding Rules

### (MISRA C:2012 – Executable Engineering Subset)

> “Comply with MISRA” is **not sufficient**.
> The following rules define the **mandatory, checkable coding subset**.

---

### 2.1 Forbidden Language Features (Mandatory)

The following **MUST NOT** be used:

* `goto`
* Direct or indirect recursion
* Variable Length Arrays (VLA)
* Runtime dynamic memory allocation:

  * `malloc`, `free`, `calloc`, `realloc`
* `setjmp` / `longjmp`
* Standard I/O:

  * `printf`, `scanf`, `sprintf`, `vsprintf`
* Unsafe string functions:

  * `strcpy`, `strcat`, `gets`

**Rationale:**
These features introduce unpredictable control flow, memory behavior, or safety risk.

---

### 2.2 Type System Rules (Type Safety)

#### 2.2.1 Integer Types

* All integer storage **MUST** use fixed-width types from `<stdint.h>`:

  * `uint8_t`, `uint16_t`, `uint32_t`
  * `int8_t`, `int16_t`, `int32_t`
* The following **MUST NOT** be used as storage types:

  * `char`
  * `int`, `long`, `short`

---

#### 2.2.2 Signed / Unsigned Mixing

* Implicit mixing of signed and unsigned types is **FORBIDDEN**
* Any required conversion **MUST**:

  * Use an explicit cast
  * Be justified with a comment

```c
/* Explicit cast required to avoid signed/unsigned mixing */
uint32_t sum = (uint32_t)a + b;
```

---

### 2.3 Expressions and Side Effects

#### 2.3.1 Hidden Side Effects (Forbidden)

A single expression **MUST NOT** contain:

* Assignment
* Increment/decrement
* Function calls with side effects

❌ Forbidden:

```c
x = y++ + func();
```

✅ Required:

```c
y++;
tmp = func();
x = y + tmp;
```

---

### 2.4 Control Flow Rules

#### 2.4.1 if / else

* All `if` and `else` bodies **MUST** use braces `{ }`
* Dangling `else` is forbidden

---

#### 2.4.2 switch

* All `switch` statements **MUST**:

  * Contain a `default` case
  * End each `case` with `break` or `return`
* Fall-through is forbidden unless:

  * Explicitly commented
  * Intentionally designed

```c
/* fall-through intentional */
```

---

### 2.5 Pointer Rules

#### 2.5.1 Pointer Usage

* Pointers may only be used for:

  * Arrays
  * Buffers
  * Explicit interface parameters
* Complex pointer arithmetic is forbidden
* Multi-level pointers (`T **`) are forbidden unless explicitly justified

---

#### 2.5.2 Null Pointer Checks

* All pointer parameters **MUST** be checked against `NULL` before use
* Code must never assume a pointer is valid

---

### 2.6 Function Design Rules

#### 2.6.1 Parameters and Return Values

* Every function **MUST**:

  * Validate all input parameters
  * Return an explicit status or error code
* Return values **MUST NOT** be ignored

```c
Std_ReturnType ret;
ret = Spi_Read(&data);
if (ret != E_OK) {
    /* error handling */
}
```

---

#### 2.6.2 Complexity Constraints

* Each function must have a single, clear responsibility
* Deep nesting is discouraged (recommendation: ≤ 3 levels)
* Excessive cyclomatic complexity is not allowed

---

### 2.7 Macros and Preprocessor Usage

#### 2.7.1 Allowed Macro Usage

Macros are allowed **ONLY** for:

* Constant definitions
* Conditional compilation
* Simple, side-effect-free expressions

Prefer `static inline` functions where possible.

---

#### 2.7.2 Macro Safety Rules

* All macro parameters **MUST** be parenthesized
* Macros **MUST NOT**:

  * Modify parameters
  * Perform assignments
  * Call functions
  * Use `++` or `--`

---

### 2.8 Initialization and Undefined Behavior

* All variables **MUST** be initialized before use
* Do not rely on default or compiler-dependent initialization
* `struct` and `array` objects must be fully initialized

---

### 2.9 Floating-Point Usage

* Floating-point arithmetic is **FORBIDDEN by default**
* If explicitly allowed:

  * No direct equality comparison (`==`)
  * Tolerance-based comparison required
  * Floating-point must not be used in ISR context

---

### 2.10 Comments and Auditability

The following **MUST** be documented with comments:

* Explicit type casts
* Rule deviations
* Intentional fall-through
* Safety-critical design decisions

Comments must explain **why the code is safe**, not what the code does.

---

### 2.11 Deviations

* Any deviation from these rules **MUST**:

  * Be explicitly marked
  * Include a safety justification
  * Be reviewable and auditable

```c
/* MISRA deviation: justified because ... */
```

---

## 3. Functional Safety Principles

### (ISO 26262–Oriented Design Rules)

---

### 3.1 Safety Mindset

Code must assume:

* Hardware can fail
* Communication can fail
* Data can be corrupted
* Timing can be violated

**Correctness must never be assumed.**

---

### 3.2 Error Handling (Mandatory)

* Errors must never be ignored
* Error handling paths must be explicit
* Error detection must lead to a defined system reaction

---

### 3.3 Safe State and Degraded Operation

For all safety-relevant functions, code must clearly define:

* What constitutes a **safe state**
* What happens on:

  * Invalid input
  * Communication failure
  * Timing violation

If system state is uncertain, code **MUST move toward a safe state**.

---

### 3.4 Plausibility and Consistency Checks

Sensor and communication data must be validated by:

* Range checks
* Plausibility checks
* Rate-of-change checks
* Stuck-at / repetition detection

CRC alone is **not sufficient**.

---

## 4. Concurrency and Execution Context

---

### 4.1 Execution Context Declaration

Every function must clearly state:

* Execution context (Task / ISR)
* Blocking behavior
* Reentrancy assumptions

ISR context restrictions:

* No blocking
* No dynamic memory
* Minimal execution time

---

### 4.2 Shared Data Protection

Shared data access must use:

* Critical sections
* Atomic operations
* RTOS primitives (if allowed)

Data ownership must be explicit.

---

## 5. Architecture and Layering

* Application code must not directly access hardware registers
* Drivers and hardware abstraction must be isolated
* Safety mechanisms must not be scattered arbitrarily

Layer violations are considered **design errors**.

---

## 6. Documentation Obligations

All generated or modified code **MUST** include:

1. **Assumptions & Constraints**

   * Context
   * Memory strategy
   * Timing assumptions
2. **Safety Considerations**

   * Failure modes
   * Detection mechanisms
   * Safe reactions
3. **Verification Checklist**

   * Static analysis expectations
   * Unit test points
   * Boundary conditions

---

## 7. Conservative Defaults Rule

If any requirement is unclear:

> **Assume the most conservative, safety-oriented interpretation.**

State assumptions explicitly instead of asking questions.

---

## 8. Non-Goals

Claude Code must **NOT**:

* Optimize for performance over safety
* Introduce clever or compact coding patterns
* Assume desktop OS behavior
* Assume unlimited resources

---

## 9. Definition of Acceptable Output

Code is acceptable only if it:

* Is readable by safety auditors
* Is analyzable by static analysis tools
* Has predictable behavior under failure
* Aligns with ISO 26262 safety philosophy

---

**End of CLAUDE.md**

---
