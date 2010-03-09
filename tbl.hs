
group sz xs = go sz xs [] []
 where
  go _ [] acc acc2     = reverse (reverse acc:acc2)
  go 0 (x:xs) acc acc2 = go sz (x:xs) [] (reverse acc:acc2)
  go n (x:xs) acc acc2 = go (n-1) xs (x:acc) acc2

pad n s | length s >= n = s
        | otherwise     = s ++ (replicate (n-length s) ' ')

main = do
  vals <- readFile "rawstats" >>= return . read
  -- min_free is same for each group in this; lots_free is same for the heads
  -- of every group
  let groups = (group 10 (vals :: [Double]))
  let a = map (\(p,v) -> (pad 16 p) ++ padded v)
                         (zip (zipWith (++) indent (map show [2,5..])) groups)
  let upper = "lots_free ->    "++concat (map (pad 8 . show) [2,5..30])++"\n"
  let table = "   min_free v\n" ++ upper ++ unlines a
  putStrLn table
 where
  padded = (concat . map (pad 8 . show))
  indent = repeat "        "
