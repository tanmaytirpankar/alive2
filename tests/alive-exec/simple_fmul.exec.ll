define float @foo(float %0) #0 {
  %2 = fmul float %0, 2.0
  ret float %2
}

; CHECK: ConcreteVal(poison=0, 32b, 6F)


define float @foo2(float %0) #0 {
  %2 = fmul ninf float %0, 2.0
  ret float %2
}

; CHECK: ConcreteVal(poison=0, 32b, 6F)
