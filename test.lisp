; a1 a2 ... an

; We must end up with n multiplications of three numbers.
; Each one must have a distinct ai in the middle.
; (* aj ai ak) where j < i and i < k.

; The neighboring elements are determined by the order of selection,
; so we can just look at the order of select elements. This is all the permutations
; of i in [1...n] so n! total possibilites.

; However, this also tells us how to estimate.


(defun smallest-index (array)
  "requires: array is non-empty"
    (prog ((smallest 0)
           (i 0)
           (N (length array)))
          loop
          (when (>= i N) 
              (return smallest))
          (when (< (aref array i) (aref array smallest))
              (setf smallest i))
          (incf i)
          (go loop)))

(defun ref (array i)
  (if (or (< i 0) (>= i  (length array)))
      1
      (aref array i)))

(defun estimate (array)
    (if (= (length array) 1)
        (aref array 0)
        (let ((i (smallest-index array)))
          (+
            (*
              (ref array i)
              (ref array (- i 1))
              (ref array (+ i 1)))

            (estimate (concatenate 'vector
                                   (subseq array 0 i) 
                                   (subseq array (+ i 1))))))))

(defun upper-bound (array)
  (loop for i from 0 below (length array) sum
        (* (reduce #'max (subseq array 0 i) :initial-value 1)
           (aref array i) 
           (reduce #'max (subseq array (+ i 1)) :initial-value 1))))

(defun upper-bound2 (array)
  (let ((N (length array))
        (x (reduce #'max array)))
    (+ (* (- N 2) (* x x x))
       (* (* x x))
       (* x))
    ))

(defun lower-bound2 (array)
  (let ((N (length array))
        (x (reduce #'min array)))
    (+ (* (- N 2) (* x x x))
       (* (* x x))
       (* x))))




(print (estimate #(5 3 2 8)))
(print (estimate #(10 10 10 10 10 10)))

;(print (upper-bound #(5 3 2 8)))



;(print (lower-bound2 #(10 10 10 10 10 10)))
;(print (upper-bound2 #(10 10 10 10 10 10)))
