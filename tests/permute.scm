(define (brute max-length set)
  (define (permute n)
    (define str (make-string n))
    (define (iter d)
      (if (= d n)
        (begin 
          (display str)
          (display " "))
        (do ((i 0 (+ i 1)))
          ((= i (string-length set)) 'done)
           (begin
             (string-set! str d (string-ref set i))
             (iter (+ d 1))))))
      (iter 0))

    (do ((len 0 (+ len 1)))
      ((> len max-length) 'done)
      (permute len)))

(brute 4 "abcd")
(newline)
