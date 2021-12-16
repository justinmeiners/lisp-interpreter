
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

(=>  (edit-distance "kitten" "sitting") 3)
