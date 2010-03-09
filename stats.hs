import System.Process

roundN :: Int -> Double -> Double
roundN n a = (fromIntegral $ round $ a*(10^n))/(10^n)

main = do
  stats <- mapM statsForRun [(min, lots) | min <- [2,5..30],lots <- [2,5..30]]
  let rounded = map (roundN 3) stats
  putStrLn $ show stats
  putStrLn $ show rounded
  writeFile "rawstats" $ show rounded

statsForRun :: (Int, Int) -> IO Double
statsForRun (min, lots) = do
  writeFile "values.h" $ "int min_free = " ++ show min ++ "; int lots_free = " ++ show lots ++ ";\n"
  runCommand "make -B" >>= waitForProcess
  runCommand "./OSP par.high" >>= waitForProcess
  runCommand "grep \"number of pagefaults\" simulation.run|awk '{print $12}' > faults.tmp" >>= waitForProcess
  runCommand "grep \"references\" simulation.run|awk '{print $7}' > refs.tmp" >>= waitForProcess
  faults <- readFile "faults.tmp" >>= return . sum . map read . lines
  refs <- readFile "refs.tmp" >>= return . sum . map read . lines
  putStrLn $ show $ (faults, refs, faults / refs)
  return $ faults / refs
