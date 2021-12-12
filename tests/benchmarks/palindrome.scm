; Find the largest palindrome made from the product of two 3-digit numbers.

; could be 6 digits
;   10^3 = 1,000 possible palindromes
; or 5 digits
;   10^3 = 1,000 possible palindromes

; 2,000 total

(define (palindrome? n)
  (define (check i str)
    (if (> (* i 2) (string-length str))
      #t
      (if (eq? (string-ref str i) 
             (string-ref str (- (string-length str) (+ i 1))))
        (check (+ i 1) str)
        #f)))
  (check 0 (number->string n)))

(define nums '())
  

(do ((i 100 (+ i 1)))
  ((> i 999) 'done)
  (do ((j 100 (+ j 1)))
    ((> j 999) 'done)
    (let ((n (* i j)))
      (if (>= n 100000)
        (if (palindrome? n)
             (push n nums)
          )))))
                            
(display (reduce max 0 nums))

