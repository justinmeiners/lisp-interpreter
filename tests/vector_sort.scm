
(define (vec-sorted? v op)
  (or (< (vector-length v) 2)
    (and (op (vector-ref v 0) (vector-ref v 1))
         (vec-sorted? (vector-tail v 1) op))))

; First make sure our sorted checker works
(assert (vec-sorted? (list->vector '(1 2 2 4 5 6)) <=))
(assert (vec-sorted? (list->vector '(1)) <=))
(assert (vec-sorted? (list->vector '(1 2)) <=))
(assert (vec-sorted? (list->vector '(7 6 5 4 3 2 1)) >=))
(assert (not (vec-sorted? (list->vector '(2 1)) <=)))
(assert (not (vec-sorted? (list->vector '(1 2 3 4 4 3)) <=)))
(assert (not (vec-sorted? (list->vector '(1 2 3 2 4 5)) <=)))

; Now test the sort function
(assert (vec-sorted? (sort! (list->vector '(1)) <) <=))
(assert (vec-sorted? (sort! (list->vector '(2 1)) <) <=))
(assert (vec-sorted? (sort! (list->vector '(1 2 3)) <) <=))
(assert (vec-sorted? (sort! (list->vector '(3 8 1 7 2 9 4 5)) <) <=))
(assert (vec-sorted? (sort! (list->vector '(1 2 3 4 5 6 7 8)) <) <=))
(assert (vec-sorted? (sort! (list->vector '(3 8 1 7 2 9 4 5)) >) >=))
(assert (vec-sorted? (sort! (list->vector '(1 2 3 4 5 6 7 8)) >) >=))
(assert (vec-sorted? (sort! (list->vector '(92 59 30 57 74 78 43 33 77 10 78 83 76 49 42 94 82 70 15 11 90 86 44 70 39 64 69 30 59 95 15 79 13 54 98 82 42 96 79 17 56 93 20 1 84 72 75 19 74 43)) >) >=))
(assert (vec-sorted? (sort! (list->vector '(92 59 30 57 74 78 43 33 77 10 78 83 76 49 42 94 82 70 15 11 90 86 44 70 39 64 69 30 59 95 15 79 13 54 98 82 42 96 79 17 56 93 20 1 84 72 75 19 74 43)) <) <=))

; This fails with 'eval error: bad argument type' -- not sure why...
;(assert (vec-sorted? (sort! (list->vector '(3 8 1 7 2 9 4 5)) <=) <=))
