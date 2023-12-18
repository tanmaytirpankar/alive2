source_filename = "/home/regehr/arm-tests/test-777947606.ll"

@bytes2 = external hidden global i32

; Function Attrs: nofree
define i32 @cast_and_load_2() #0 {
  %1 = load i32, ptr @bytes2, align 4
  %2 = urem i32 66690555, 1283691433
  store i32 %2, ptr @bytes2, align 4
  ret i32 %1
}

attributes #0 = { nofree }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"uwtable", i32 1}
!2 = !{!"clang version 13.0.0"}
