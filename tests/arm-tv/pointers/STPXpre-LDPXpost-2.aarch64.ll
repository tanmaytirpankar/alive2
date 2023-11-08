; ModuleID = 'M1'
source_filename = "M1"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linux-gnu"

define signext i51 @f(i51 zeroext %0, i51 zeroext %1, i51 noundef zeroext %2, i51 noundef zeroext %3) {
  %5 = icmp ule i51 1, %2
  %6 = select i1 %5, i51 1, i51 %0
  %7 = icmp ugt i51 %1, %3
  %8 = select i1 %5, i51 %1, i51 %0
  %9 = trunc i51 %0 to i1
  %10 = select i1 %9, i51 %0, i51 %1
  %maskA = trunc i51 %1 to i6
  %maskB = sext i6 %maskA to i51
  %11 = lshr i51 %6, %1
  %12 = icmp sle i51 %0, %2
  %13 = freeze i1 %5
  %14 = zext i1 %13 to i51
  %15 = call i51 @llvm.fshr.i51(i51 %14, i51 %3, i51 %11)
  %16 = freeze i1 %5
  %17 = zext i1 %16 to i51
  %18 = select i1 %12, i51 270582939648, i51 %17
  %19 = icmp ugt i51 26678, %2
  %20 = zext i1 %5 to i51
  %maskC = and i51 %11, 63
  %21 = and i51 %20, %11
  %22 = trunc i51 %0 to i1
  %23 = select i1 %22, i51 %18, i51 %6
  %maskC1 = and i51 %15, 63
  %24 = lshr exact i51 %11, %maskC1
  %25 = trunc i51 %1 to i1
  %26 = select i1 %25, i51 %15, i51 %18
  %maskA2 = trunc i51 %18 to i6
  %maskB3 = zext i6 %maskA2 to i51
  %27 = srem i51 %1, %18
  %28 = sext i1 %5 to i51
  %29 = call i51 @llvm.sadd.sat.i51(i51 %28, i51 %26)
  %30 = trunc i51 %18 to i1
  %31 = select i1 %30, i51 %15, i51 %6
  %32 = sext i1 %5 to i51
  %33 = select i1 %5, i51 %8, i51 %32
  %34 = icmp sgt i51 %8, %1
  %35 = freeze i51 %31
  %36 = trunc i51 %35 to i1
  %37 = select i1 %36, i51 %33, i51 %1
  %maskA4 = trunc i51 %23 to i6
  %maskB5 = zext i6 %maskA4 to i51
  %38 = mul nuw nsw i51 %8, %23
  %39 = icmp sgt i51 %24, %38
  %40 = sext i1 %12 to i51
  %41 = zext i1 %12 to i51
  %42 = icmp slt i51 %40, %41
  %43 = zext i1 %12 to i51
  %maskC6 = and i51 %43, 63
  %44 = srem i51 %6, %43
  %45 = select i1 false, i51 %23, i51 %38
  %46 = call i51 @llvm.smin.i51(i51 %1, i51 %1)
  %47 = call i51 @llvm.usub.sat.i51(i51 %29, i51 %21)
  %48 = icmp ugt i51 %23, %0
  %49 = icmp sgt i51 %27, %37
  %50 = zext i1 %5 to i51
  %maskC7 = and i51 %50, 63
  %51 = or i51 %6, %50
  %52 = sext i1 %7 to i51
  %53 = call i51 @llvm.fshl.i51(i51 %45, i51 %11, i51 %52)
  %54 = call i51 @llvm.ushl.sat.i51(i51 %46, i51 %26)
  %55 = trunc i51 %8 to i1
  %56 = select i1 %55, i51 %54, i51 %1
  %57 = zext i1 %49 to i51
  %58 = call i51 @llvm.ctlz.i51(i51 %57, i1 true)
  %59 = sext i1 %5 to i51
  %60 = icmp sle i51 %33, %59
  %61 = freeze i1 %34
  %62 = zext i1 %61 to i51
  %63 = trunc i51 %53 to i1
  %64 = select i1 %63, i51 %31, i51 %62
  %65 = select i1 %49, i51 %33, i51 %38
  %66 = icmp ult i51 %31, %1
  %67 = sext i1 %49 to i51
  %68 = sext i1 %48 to i51
  %69 = call i51 @llvm.fshr.i51(i51 %67, i51 %6, i51 %68)
  %70 = freeze i51 %27
  %71 = trunc i51 %31 to i1
  %72 = select i1 %71, i51 %0, i51 %70
  %73 = sext i1 %42 to i51
  %74 = zext i1 %19 to i51
  %maskA8 = trunc i51 %74 to i6
  %maskB9 = sext i6 %maskA8 to i51
  %75 = lshr i51 %73, %maskB9
  %76 = zext i1 %48 to i51
  %77 = icmp sgt i51 %76, %1
  %78 = freeze i1 %5
  %79 = zext i1 %78 to i51
  %80 = sext i1 %48 to i51
  %81 = icmp uge i51 %79, %80
  %82 = freeze i51 %54
  %83 = call i51 @llvm.cttz.i51(i51 %82, i1 false)
  %84 = zext i1 %60 to i51
  %85 = freeze i51 %0
  %maskC10 = and i51 %85, 63
  %86 = udiv exact i51 %84, %85
  %87 = call i51 @llvm.abs.i51(i51 %26, i1 true)
  %88 = freeze i51 %0
  %89 = icmp uge i51 %88, %83
  %90 = freeze i51 %64
  %91 = icmp sge i51 %90, %58
  %92 = call i51 @llvm.ctlz.i51(i51 %23, i1 true)
  %93 = zext i1 %89 to i51
  %94 = select i1 %81, i51 %93, i51 %26
  %maskC11 = and i51 %87, 63
  %95 = and i51 %11, %87
  %96 = trunc i51 %47 to i1
  %97 = select i1 %96, i51 60234, i51 %0
  %maskA12 = trunc i51 %24 to i6
  %maskB13 = sext i6 %maskA12 to i51
  %98 = sdiv exact i51 %37, %24
  %maskA14 = trunc i51 %6 to i6
  %maskB15 = zext i6 %maskA14 to i51
  %99 = xor i51 %75, %6
  %100 = freeze i51 %72
  %101 = call i51 @llvm.abs.i51(i51 %100, i1 false)
  %102 = zext i1 %89 to i51
  %103 = icmp ugt i51 %33, %102
  %104 = zext i1 %77 to i51
  %105 = icmp sle i51 %104, %31
  %106 = sext i1 %7 to i51
  %107 = sext i1 %5 to i51
  %108 = icmp sle i51 %106, %107
  %109 = zext i1 %7 to i51
  %110 = freeze i51 %83
  %111 = icmp ule i51 %109, %110
  %112 = freeze i51 %65
  %113 = freeze i1 %48
  %114 = sext i1 %113 to i51
  %115 = icmp uge i51 %112, %114
  %116 = icmp sgt i51 %75, %98
  %117 = zext i1 %60 to i51
  %118 = trunc i51 %51 to i1
  %119 = select i1 %118, i51 %117, i51 %101
  %120 = icmp uge i51 %95, %99
  %121 = zext i1 %115 to i51
  %maskA16 = trunc i51 %6 to i6
  %maskB17 = sext i6 %maskA16 to i51
  %122 = sdiv i51 %121, %6
  %123 = zext i1 %89 to i51
  %maskC18 = and i51 %123, 63
  %124 = ashr exact i51 %94, %123
  %125 = icmp sle i51 %69, %97
  %126 = sext i1 %111 to i51
  %127 = trunc i51 %58 to i1
  %128 = select i1 %127, i51 %51, i51 %126
  %129 = freeze i51 %53
  %130 = icmp ne i51 %65, %129
  %131 = call i51 @llvm.fshr.i51(i51 %101, i51 %97, i51 %46)
  %132 = trunc i51 %51 to i1
  %133 = select i1 %132, i51 %6, i51 %47
  %134 = zext i1 %130 to i51
  %135 = select i1 %39, i51 %134, i51 %124
  %136 = call i51 @llvm.fshr.i51(i51 %75, i51 %83, i51 %1)
  %137 = icmp ugt i51 %128, %95
  %138 = zext i1 %130 to i51
  %139 = sext i1 %34 to i51
  %140 = trunc i51 %75 to i1
  %141 = select i1 %140, i51 %138, i51 %139
  %142 = zext i1 %116 to i51
  %maskA19 = trunc i51 %101 to i6
  %maskB20 = sext i6 %maskA19 to i51
  %143 = udiv i51 %142, %101
  %maskC21 = and i51 %33, 63
  %144 = ashr i51 %21, %maskC21
  %145 = sext i1 %19 to i51
  %146 = sext i1 %66 to i51
  %maskA22 = trunc i51 %146 to i6
  %maskB23 = sext i6 %maskA22 to i51
  %147 = mul nsw i51 %145, %146
  %148 = call i51 @llvm.fshl.i51(i51 %33, i51 %21, i51 %124)
  %149 = icmp eq i51 %147, %45
  %150 = select i1 %60, i51 %97, i51 %98
  %151 = sext i1 %7 to i51
  %152 = zext i1 %81 to i51
  %153 = trunc i51 %72 to i1
  %154 = select i1 %153, i51 %151, i51 %152
  %155 = sext i1 %89 to i51
  %156 = trunc i51 %122 to i1
  %157 = select i1 %156, i51 %64, i51 %155
  %158 = freeze i1 %77
  %159 = zext i1 %158 to i51
  %160 = freeze i51 %128
  %161 = trunc i51 %160 to i1
  %162 = select i1 %161, i51 %135, i51 %159
  %163 = freeze i51 %162
  %164 = trunc i51 %95 to i1
  %165 = select i1 %164, i51 %46, i51 %163
  %maskA24 = trunc i51 %29 to i6
  %maskB25 = zext i6 %maskA24 to i51
  %166 = mul nuw i51 %15, %29
  %maskA26 = trunc i51 %2 to i6
  %maskB27 = sext i6 %maskA26 to i51
  %167 = lshr exact i51 562949953447990, %maskB27
  %168 = sext i1 %116 to i51
  %169 = trunc i51 %75 to i1
  %170 = select i1 %169, i51 %166, i51 %168
  %171 = zext i1 %108 to i51
  %172 = icmp eq i51 %171, %92
  %173 = freeze i51 %29
  %174 = freeze i1 %42
  %175 = sext i1 %174 to i51
  %176 = call { i51, i1 } @llvm.sadd.with.overflow.i51(i51 %173, i51 %175)
  %177 = extractvalue { i51, i1 } %176, 1
  %178 = extractvalue { i51, i1 } %176, 0
  %179 = icmp ult i51 %51, %33
  %180 = sext i1 %179 to i51
  %maskA28 = trunc i51 %124 to i6
  %maskB29 = zext i6 %maskA28 to i51
  %181 = add nuw i51 %180, %124
  %182 = sext i1 %105 to i51
  %183 = trunc i51 %64 to i1
  %184 = select i1 %183, i51 %182, i51 440177133373556
  %185 = sext i1 %108 to i51
  %186 = call { i51, i1 } @llvm.usub.with.overflow.i51(i51 %185, i51 %166)
  %187 = extractvalue { i51, i1 } %186, 1
  %188 = extractvalue { i51, i1 } %186, 0
  %189 = icmp slt i51 %47, %64
  %190 = zext i1 %91 to i51
  %maskA30 = trunc i51 %178 to i6
  %maskB31 = sext i6 %maskA30 to i51
  %191 = srem i51 %190, %178
  %192 = sext i1 %81 to i51
  %193 = freeze i51 %53
  %194 = icmp slt i51 %192, %193
  %195 = zext i1 %179 to i51
  %196 = icmp uge i51 %6, %195
  %197 = call i51 @llvm.ctlz.i51(i51 %135, i1 true)
  ret i51 %141
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.fshr.i51(i51, i51, i51) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.sadd.sat.i51(i51, i51) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.smin.i51(i51, i51) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.usub.sat.i51(i51, i51) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.fshl.i51(i51, i51, i51) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.ushl.sat.i51(i51, i51) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.ctlz.i51(i51, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.cttz.i51(i51, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i51 @llvm.abs.i51(i51, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i51, i1 } @llvm.sadd.with.overflow.i51(i51, i51) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i51, i1 } @llvm.usub.with.overflow.i51(i51, i51) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
