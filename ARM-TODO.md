# ARM-TV TODO

- assert that stack memory is not trashed
  - saved ret addr
  - saved FP
  - what else?
- stop using instrs_64 and friends, just use the register numbers
  in the instructions to get the correct-sized operands
- support modules that contain more than one function
  - should be easy, just requires some refactoring to separate
    per-module code (such as processing global variables) from
    per-function code
- support function calls
- size the stack correctly, instead of hard-coding a size
  - https://llvm.org/doxygen/classllvm_1_1AArch64FunctionInfo.html
  - https://llvm.org/docs/StackMaps.html
- write a script that finds test cases for instructions where we lack a test
- find a good set of opt passes that doesn't trigger middle-end FP bugs
  - then turn FP back on and keep adding instructions
- mark parameters noundef

- more vector instructions
- more memory operations
- more FP operations
