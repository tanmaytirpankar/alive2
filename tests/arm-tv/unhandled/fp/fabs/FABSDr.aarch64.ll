define double @absd(double %0) {
  %2 = bitcast double %0 to i64
  %3 = and i64 %2, 9223372036854775807
  %4 = bitcast i64 %3 to double
  ret double %4
}
