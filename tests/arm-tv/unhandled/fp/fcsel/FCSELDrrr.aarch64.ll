define double @f(i64 %0) {
  %2 = icmp eq i64 %0, 0
  %3 = uitofp i1 %2 to double
  ret double %3
}
