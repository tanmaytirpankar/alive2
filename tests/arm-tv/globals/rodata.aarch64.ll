; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

@.str.4288 = constant [8 x i8] c"jmp\09%P1\00"

; Function Attrs: strictfp
define ptr @output_698(ptr %insn) #0 {
entry:
  %bf.load = load i32, ptr %insn, align 8
  %tobool.not = icmp eq i32 %bf.load, 0
  %.str.4289..str.4288 = select i1 %tobool.not, ptr null, ptr @.str.4288
  ret ptr %.str.4289..str.4288
}

attributes #0 = { strictfp }
