; Test tail call optimization

(def peano
  (fn (a b)
    (if (= a 0)
      b
      (peano (- a 1) (+ b 1)))))

(display (peano 100 200))
 
