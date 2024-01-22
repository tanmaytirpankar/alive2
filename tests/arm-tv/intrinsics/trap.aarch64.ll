define void @test-sext-sub(ptr %input, i32 %sub, i32 %numIterations) {
entry:
  br label %loop
loop:
  %i = phi i32 [ %nexti, %cont ], [ 0, %entry ]

  %ssub = tail call { i32, i1 } @llvm.ssub.with.overflow.i32(i32 %i, i32 %sub)
  %val = extractvalue { i32, i1 } %ssub, 0
  %ovfl = extractvalue { i32, i1 } %ssub, 1
  br i1 %ovfl, label %trap, label %cont

trap:
  tail call void @llvm.trap()
  unreachable

cont:
  %index64 = sext i32 %val to i64

  %ptr = getelementptr inbounds float, ptr %input, i64 %index64
  %nexti = add nsw i32 %i, 1
  %f = load float, ptr %ptr, align 4
  %exitcond = icmp eq i32 %nexti, %numIterations
  br i1 %exitcond, label %exit, label %loop
exit:
  ret void
}

declare void @llvm.trap()
