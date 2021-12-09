
(define v #(1 2 3))
(vector-swap! v 0 2)
(assert (= 3 (vector-ref v 0)))
(assert (= 1 (vector-ref v 2)))

(define (vec-sorted? v op)
 ; "if x and y are any two adjacent elements in the result,
 ;  where x precedes y, it is the case that (procedure y x) => #f"
 ; https://groups.csail.mit.edu/mac/ftpdir/scheme-7.4/doc-html/scheme_8.html#SEC72

 (or (< (vector-length v) 2)
  (and (not (op (vector-ref v 1) (vector-ref v 0)))
   (vec-sorted? (vector-tail v 1) op))))

; First make sure our sorted checker works
(assert (vec-sorted? #(1 2 2 4 5 6) <))
(assert (vec-sorted? #(1) <))
(assert (vec-sorted? #(1 2) <))
(assert (vec-sorted? #(7 6 5 4 3 2 1) >))
(assert (not (vec-sorted? #(2 1) <)))
(assert (not (vec-sorted? #(1 2 3 4 4 3) <)))
(assert (not (vec-sorted? #(1 2 3 2 4 5) <)))

; Now test the sort function
(assert (vec-sorted? (sort! #(1) <) <))
(assert (vec-sorted? (sort! #(2 1) <) <))
(assert (vec-sorted? (sort! #(1 2 3) <) <))
(assert (vec-sorted? (sort! #(3 8 1 7 2 9 4 5) <) <))
(assert (vec-sorted? (sort! #(1 2 3 4 5 6 7 8) <) <))
(assert (vec-sorted? (sort! #(3 8 1 7 2 9 4 5) >) >))
(assert (vec-sorted? (sort! #(1 2 3 4 5 6 7 8) >) >))
(assert (vec-sorted? (sort! #(92 59 30 57 74 78 43 33 77 10 78 83 76 49 42 94 82 70 15 11 90 86 44 70 39 64 69 30 59 95 15 79 13 54 98 82 42 96 79 17 56 93 20 1 84 72 75 19 74 43) >) >))
(assert (vec-sorted? (sort! #(92 59 30 57 74 78 43 33 77 10 78 83 76 49 42 94 82 70 15 11 90 86 44 70 39 64 69 30 59 95 15 79 13 54 98 82 42 96 79 17 56 93 20 1 84 72 75 19 74 43) <) <))
(assert (vec-sorted? (sort! #(3 8 1 7 2 9 4 5) <) <))

; Try other data types
(assert (vec-sorted? (sort! #(#\C #\B #\A #\D) char<?) char<?))

