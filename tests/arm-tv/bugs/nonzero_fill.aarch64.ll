; ModuleID = 'reduced.bc'
target datalayout = "e-m:o-i64:64-i128:128-n32:64-S128"
target triple = "arm64-apple-macosx14.0.0"

%struct.uiWidgetColors = type { [4 x i8], [4 x i8], [4 x i8], [4 x i8], [4 x i8], [4 x i8], i16, i16, i16, i16 }

@wcol_tool = constant %struct.uiWidgetColors { [4 x i8] c"\19\19\19\FF", [4 x i8] c"\99\99\99\FF", [4 x i8] c"ddd\FF", [4 x i8] c"\19\19\19\FF", [4 x i8] c"\00\00\00\FF", [4 x i8] c"\FF\FF\FF\FF", i16 1, i16 15, i16 -15, i16 0 }

; Function Attrs: strictfp
define void @ui_widget_color_init() #0 {
entry:
  ret void
}

attributes #0 = { strictfp }
