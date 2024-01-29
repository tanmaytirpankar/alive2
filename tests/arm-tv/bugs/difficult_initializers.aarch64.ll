target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

%struct.StructRNA = type { %struct.ContainerRNA, ptr, ptr, ptr, i32, ptr, ptr, ptr, i32, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, %struct.ListBase }
%struct.ContainerRNA = type { ptr, ptr, ptr, %struct.ListBase }
%struct.ListBase = type { ptr, ptr }
%struct.PointerPropertyRNA = type { %struct.PropertyRNA, ptr, ptr, ptr, ptr, ptr }
%struct.PropertyRNA = type { ptr, ptr, i32, ptr, i32, ptr, ptr, i32, ptr, i32, i32, ptr, i32, [3 x i32], i32, ptr, i32, ptr, ptr, i32, i32, ptr, ptr }
%struct.FunctionRNA = type { %struct.ContainerRNA, ptr, i32, ptr, ptr, ptr }

@.str.2 = private constant [1 x i8] zeroinitializer
@RNA_Header = global %struct.StructRNA { %struct.ContainerRNA { ptr @RNA_Menu, ptr null, ptr null, %struct.ListBase zeroinitializer }, ptr null, ptr null, ptr null, i32 4, ptr null, ptr null, ptr null, i32 17, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, ptr null, %struct.ListBase { ptr @rna_Header_draw_func, ptr @rna_Header_draw_func } }
@rna_Header_draw_context = global %struct.PointerPropertyRNA { %struct.PropertyRNA { ptr null, ptr null, i32 -1, ptr null, i32 8388612, ptr @.str.2, ptr @.str.2, i32 0, ptr null, i32 5, i32 0, ptr null, i32 0, [3 x i32] zeroinitializer, i32 0, ptr null, i32 0, ptr null, ptr null, i32 0, i32 -1, ptr null, ptr null }, ptr null, ptr null, ptr null, ptr null, ptr null }
@rna_Header_draw_func = global %struct.FunctionRNA { %struct.ContainerRNA { ptr null, ptr null, ptr null, %struct.ListBase { ptr @rna_Header_draw_context, ptr @rna_Header_draw_context } }, ptr null, i32 32, ptr null, ptr null, ptr null }
@RNA_Menu = global %struct.StructRNA { %struct.ContainerRNA { ptr null, ptr @RNA_Header, ptr null, %struct.ListBase zeroinitializer }, ptr null, ptr null, ptr null, i32 4, ptr null, ptr null, ptr null, i32 17, ptr null, ptr null, ptr null, ptr null, ptr @rna_Menu_refine, ptr null, ptr null, ptr null, ptr null, ptr null, %struct.ListBase zeroinitializer }

; Function Attrs: strictfp
define ptr @rna_Menu_refine() #0 {
entry:
  ret ptr @RNA_Menu
}

; uselistorder directives
uselistorder ptr @rna_Header_draw_func, { 1, 0 }

attributes #0 = { strictfp }
