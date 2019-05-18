; PROBLEW
; W - maximum bag weight
; {m_1, m_2, ... m_n} = item weights
; {v_1, v_2, ... v_n} = item values

; want to choose a subset I so that
; sum m_i <= W
; and
; sum v_i is maximized
; In other words, if we choose another subset J
; then sum v_j <= sum v_i


(define (rand-item max-weight max-cost)
  (cons (pseudo-rand max-weight)
        (pseudo-rand max-cost)))

(define (build-items n)
  (if (= n 0)
      '()
      (cons (rand-item 100 100)
        (build-items (- n 1)))))


(define (display-items ls)
  (if (null? ls)
      '()
      (begin
        (display (car (car ls)))
        (display ", ")
        (display (cdr (car ls)))
        (newline)
        (display-items (cdr ls)))))


;(define items '((23 505) (26 352) (18 220) (32 354) (27 414) (29 498) (26 545) (30 473) (27 543)))

(pseudo-seed! (+ 42 128))
(define items (build-items 20))
;(display-items items)

(define (reduce f accum ls)
    (if (null? ls)
        accum
        (reduce f (f accum (car ls)) (cdr ls))))

(define (max . ls)
  (reduce (lambda (m x) 
            (if (> x m)
                x
                m)) (car ls) (cdr ls)))

(define (knapsack remaining items)
  (if (or (null? items) (<= remaining 0))
    0
    (let ((weight (car (car items)))
           (val (cdr (car items))))
      (max
        (if (>= (- remaining weight) 0)
          (+ val (knapsack (- remaining weight) (cdr items)))
          0)
        (knapsack remaining (cdr items))))))

(display (knapsack 700 items))


