; canada
(let ((data (read)))
    (gc-flip)
    (print-gc-statistics)
    (newline)
    (display "records: ")
    (display (vector-length data)))

(gc-flip)
(print-gc-statistics)

 
