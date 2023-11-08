; ModuleID = 'M1'
source_filename = "M1"
target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-linux-gnu"

define i40 @f(i40 signext %0) {
  %maskC = and i40 %0, 63
  %2 = sub i40 -206158430207, %0
  %3 = select i1 true, i40 %2, i40 -137438954496
  %maskA = trunc i40 %3 to i6
  %maskB = zext i6 %maskA to i40
  %4 = lshr exact i40 %2, %3
  %5 = icmp ne i40 -137438954488, %2
  %6 = icmp ne i40 %4, %0
  %maskC1 = and i40 %4, 63
  %7 = ashr exact i40 -412176334334, %maskC1
  %8 = sext i1 %6 to i40
  %maskA2 = trunc i40 %2 to i6
  %maskB3 = zext i6 %maskA2 to i40
  %9 = lshr exact i40 %8, %2
  %10 = sext i1 %5 to i40
  %maskA4 = trunc i40 %10 to i6
  %maskB5 = sext i6 %maskA4 to i40
  %11 = lshr exact i40 %2, %10
  %12 = sext i1 %5 to i40
  %13 = icmp ne i40 %0, %12
  %maskC6 = and i40 %2, 63
  %14 = xor i40 %7, %2
  %15 = zext i1 %5 to i40
  %16 = select i1 %13, i40 %15, i40 %7
  %17 = sext i1 %13 to i40
  %maskA7 = trunc i40 %17 to i6
  %maskB8 = zext i6 %maskA7 to i40
  %18 = xor i40 %4, %17
  %19 = call i40 @llvm.fshr.i40(i40 %18, i40 %9, i40 %7)
  %20 = call i40 @llvm.fshl.i40(i40 %16, i40 %3, i40 %7)
  %21 = sext i1 %6 to i40
  %22 = call i40 @llvm.fshr.i40(i40 %19, i40 %7, i40 %21)
  %23 = freeze i1 %6
  %24 = zext i1 %23 to i40
  %25 = freeze i40 %7
  %26 = freeze i40 %25
  %maskA9 = trunc i40 %26 to i6
  %maskB10 = sext i6 %maskA9 to i40
  %27 = sdiv exact i40 %24, %25
  %28 = icmp sgt i40 %14, %4
  %29 = trunc i40 %3 to i1
  %30 = select i1 %29, i40 %4, i40 %14
  %31 = freeze i40 %20
  %32 = trunc i40 %3 to i1
  %33 = select i1 %32, i40 %31, i40 %20
  %34 = zext i1 %13 to i40
  %35 = trunc i40 %4 to i1
  %36 = select i1 %35, i40 %34, i40 %19
  %37 = zext i1 %13 to i40
  %38 = sext i1 %5 to i40
  %39 = sext i1 %6 to i40
  %40 = call i40 @llvm.fshl.i40(i40 %37, i40 %38, i40 %39)
  %41 = select i1 %28, i40 %18, i40 %22
  %maskA11 = trunc i40 %27 to i6
  %maskB12 = zext i6 %maskA11 to i40
  %42 = urem i40 %33, %27
  %43 = sext i1 %5 to i40
  %44 = freeze i1 %5
  %45 = sext i1 %44 to i40
  %46 = call i40 @llvm.fshl.i40(i40 %43, i40 %33, i40 %45)
  %maskA13 = trunc i40 137438954495 to i6
  %maskB14 = zext i6 %maskA13 to i40
  %47 = srem i40 %7, 137438954495
  %maskC15 = and i40 -137573172224, 63
  %48 = urem i40 %42, -137573172224
  %49 = icmp slt i40 %27, %30
  %50 = icmp ugt i40 %16, %3
  %maskA16 = trunc i40 %30 to i6
  %maskB17 = zext i6 %maskA16 to i40
  %51 = and i40 %7, %30
  %52 = icmp eq i40 %19, -411907898878
  %53 = select i1 %50, i40 %22, i40 %20
  %maskC18 = and i40 %41, 63
  %54 = shl nsw i40 %53, %41
  %55 = sext i1 %50 to i40
  %56 = icmp uge i40 %47, %55
  %57 = sext i1 %28 to i40
  %maskC19 = and i40 %57, 63
  %58 = or i40 %11, %57
  %59 = call i40 @llvm.abs.i40(i40 %40, i1 true)
  %60 = freeze i40 %19
  %61 = icmp sle i40 %20, %60
  %62 = sext i1 %50 to i40
  %63 = icmp slt i40 %59, %62
  %64 = icmp ule i40 %3, %18
  %65 = call i40 @llvm.umin.i40(i40 %41, i40 %47)
  %66 = zext i1 %49 to i40
  %maskA20 = trunc i40 %66 to i6
  %maskB21 = zext i6 %maskA20 to i40
  %67 = mul i40 %19, %66
  %68 = zext i1 %64 to i40
  %69 = call { i40, i1 } @llvm.uadd.with.overflow.i40(i40 %53, i40 %68)
  %70 = extractvalue { i40, i1 } %69, 1
  %71 = extractvalue { i40, i1 } %69, 0
  %72 = zext i1 %50 to i40
  %73 = sext i1 %63 to i40
  %maskA22 = trunc i40 %73 to i6
  %maskB23 = sext i6 %maskA22 to i40
  %74 = or i40 %72, %73
  %maskC24 = and i40 %0, 63
  %75 = ashr exact i40 %42, %0
  %76 = freeze i40 %46
  %77 = icmp ugt i40 %27, %76
  %78 = icmp sgt i40 %2, %27
  %79 = call i40 @llvm.smax.i40(i40 %51, i40 %48)
  %80 = zext i1 %61 to i40
  %81 = call i40 @llvm.fshr.i40(i40 %67, i40 %80, i40 %47)
  %82 = zext i1 %50 to i40
  %83 = call i40 @llvm.usub.sat.i40(i40 %14, i40 %82)
  %84 = freeze i40 %30
  %85 = zext i1 %13 to i40
  %86 = call i40 @llvm.smin.i40(i40 %84, i40 %85)
  %87 = sext i1 %50 to i40
  %88 = trunc i40 %58 to i1
  %89 = select i1 %88, i40 %87, i40 %79
  %90 = zext i1 %61 to i40
  %91 = icmp eq i40 %90, %71
  %92 = zext i1 %63 to i40
  %93 = trunc i40 %59 to i1
  %94 = select i1 %93, i40 %58, i40 %92
  %95 = sext i1 %56 to i40
  %96 = icmp eq i40 %95, %94
  %97 = zext i1 %91 to i40
  %98 = call i40 @llvm.fshl.i40(i40 %97, i40 %47, i40 %48)
  %99 = freeze i1 %78
  %100 = sext i1 %99 to i40
  %101 = trunc i40 %47 to i1
  %102 = select i1 %101, i40 %40, i40 %100
  %103 = call i40 @llvm.fshr.i40(i40 %11, i40 %40, i40 -137438954492)
  %104 = sext i1 %77 to i40
  %maskA25 = trunc i40 %89 to i6
  %maskB26 = zext i6 %maskA25 to i40
  %105 = ashr i40 %104, %maskB26
  %106 = icmp slt i40 %67, %2
  %107 = zext i1 %96 to i40
  %108 = icmp ult i40 %102, %107
  %109 = sext i1 %56 to i40
  %110 = select i1 %56, i40 %58, i40 %109
  %111 = call i40 @llvm.ctlz.i40(i40 %48, i1 false)
  %112 = call i40 @llvm.cttz.i40(i40 %74, i1 false)
  %113 = zext i1 %56 to i40
  %114 = select i1 %56, i40 %113, i40 %51
  %115 = icmp ule i40 %105, %54
  %116 = freeze i40 %67
  %maskA27 = trunc i40 %102 to i6
  %maskB28 = zext i6 %maskA27 to i40
  %117 = urem i40 %116, %102
  %118 = call { i40, i1 } @llvm.smul.with.overflow.i40(i40 %105, i40 %9)
  %119 = extractvalue { i40, i1 } %118, 1
  %120 = extractvalue { i40, i1 } %118, 0
  %121 = call i40 @llvm.ushl.sat.i40(i40 %59, i40 %22)
  %122 = zext i1 %5 to i40
  %123 = call i40 @llvm.cttz.i40(i40 %122, i1 true)
  %124 = zext i1 %106 to i40
  %125 = select i1 %50, i40 %124, i40 %4
  %126 = icmp ule i40 %67, %51
  %127 = freeze i1 %77
  %128 = zext i1 %127 to i40
  %129 = trunc i40 %71 to i1
  %130 = select i1 %129, i40 %83, i40 %128
  %131 = sext i1 %77 to i40
  %132 = select i1 %119, i40 %131, i40 %105
  %133 = sext i1 %49 to i40
  %134 = sext i1 %106 to i40
  %135 = icmp uge i40 %133, %134
  %136 = sext i1 %108 to i40
  %137 = icmp sge i40 %136, %11
  %138 = icmp slt i40 %18, %2
  %139 = sext i1 %137 to i40
  %140 = trunc i40 %51 to i1
  %141 = select i1 %140, i40 %139, i40 %102
  %142 = trunc i40 %67 to i1
  %143 = select i1 %142, i40 %14, i40 %71
  %144 = trunc i40 %105 to i1
  %145 = select i1 %144, i40 %4, i40 %117
  %146 = trunc i40 %16 to i1
  %147 = select i1 %146, i40 %114, i40 %132
  %148 = sext i1 %115 to i40
  %149 = select i1 %108, i40 %105, i40 %148
  %maskA29 = trunc i40 %30 to i6
  %maskB30 = sext i6 %maskA29 to i40
  %150 = add nsw i40 %79, %30
  %151 = trunc i40 %147 to i1
  %152 = select i1 %151, i40 %47, i40 %65
  %maskC31 = and i40 %22, 63
  %153 = or i40 %11, %22
  %154 = call i40 @llvm.fshr.i40(i40 %2, i40 %114, i40 %94)
  %155 = call i40 @llvm.fshl.i40(i40 %2, i40 %123, i40 %4)
  %156 = sext i1 %6 to i40
  %157 = freeze i40 %98
  %158 = trunc i40 %157 to i1
  %159 = select i1 %158, i40 %156, i40 %111
  %160 = call i40 @llvm.fshl.i40(i40 %159, i40 %11, i40 %71)
  %161 = sext i1 %63 to i40
  %162 = freeze i40 %125
  %163 = sext i1 %115 to i40
  %164 = call i40 @llvm.fshr.i40(i40 %161, i40 %162, i40 %163)
  %165 = sext i1 %78 to i40
  %166 = icmp sgt i40 %53, %165
  %167 = sext i1 %108 to i40
  %168 = select i1 %52, i40 %111, i40 %167
  %169 = trunc i40 %3 to i1
  %170 = select i1 %169, i40 %105, i40 %14
  %171 = trunc i40 %153 to i1
  %172 = select i1 %171, i40 %168, i40 %48
  %173 = trunc i40 %110 to i1
  %174 = select i1 %173, i40 %42, i40 %159
  %maskC32 = and i40 %94, 63
  %175 = sdiv i40 %94, %94
  ret i40 %164
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.fshr.i40(i40, i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.fshl.i40(i40, i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.abs.i40(i40, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.umin.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i40, i1 } @llvm.uadd.with.overflow.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.smax.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.usub.sat.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.smin.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.ctlz.i40(i40, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.cttz.i40(i40, i1 immarg) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i40, i1 } @llvm.smul.with.overflow.i40(i40, i40) #0

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i40 @llvm.ushl.sat.i40(i40, i40) #0

attributes #0 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
