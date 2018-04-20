
; check that we can load an interact with this file

(let ((data (read-path "big_data_gen.sexpr")))
    (display "records: ")
    (display (length data))
    (newline)
    (let ((record (car data)))
        (assert (= (nth 1 (assoc record 'index)) 0)
        (assert (eq? (nth 1 (assoc record 'isActive)) 'True)))
        (assert (= (nth 1 (assoc record 'age)) 26))))
   
(display "done")
(newline)

(let ((data (read-path "big_data_canada.sexpr")))
    (display "records: ")
    (display (length data)))

