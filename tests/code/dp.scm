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
  (cons (random max-weight)
        (random max-cost)))

(define (build-items n)
  (if (= n 0)
    '()
    (cons (rand-item 100 100)
          (build-items (- n 1)))))
;(random-seed! (GET-UNIVERSAL-TIME))
;(define items (build-items 10))


(define items '((23 . 505) (26 . 352) (18 . 220) (32 . 354) (27 . 414) (29 . 498) (26 . 545) (30 . 473) (27 . 543)))

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

(display (knapsack 67 items))

(assert (= (knapsack 67 items) 1270))


; https://en.wikipedia.org/wiki/Levenshtein_distance
(define (edit-distance-list a b eq?)
  (cond ((null? a) (length b))
		((null? b) (length a))
		(else (min
				(+ 1 (edit-distance-list (cdr a) b eq?)) ; insert
				(+ 1 (edit-distance-list a (cdr b) eq?)) ; delete
				(+ 
				  (if (eq? (car a) (car b)) 0 1) ; replace if needed
				  (edit-distance-list (cdr a) (cdr b) eq?))))))

(define (edit-distance a b)
  (edit-distance-list (string->list a) (string->list b) char=?))

(==>  (edit-distance "kitten" "sitting") 3) 
