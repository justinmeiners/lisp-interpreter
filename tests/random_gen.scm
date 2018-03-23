
; check that we can load an interact with this file

(let ((data (read-path "random_gen.sexpr")))
    (display "records: ")
    (display (length data))
    (newline)
    (let ((record (car data)))
        (assert (eq? (nth 1 (assoc record 'index)) 0)
        (assert (eq? (nth 1 (assoc record 'isActive)) 'True)))
        (assert (eq? (nth 1 (assoc record 'age)) 26))))
   
(display "done")
(newline)
