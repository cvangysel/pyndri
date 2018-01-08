import pyndri

print('predictions',
      pyndri.krovetz_stem('predictions'),
      pyndri.porter_stem('predictions'))  # prediction, predict
print('marketing',
      pyndri.krovetz_stem('marketing'),
      pyndri.porter_stem('marketing'))  # marketing, market
print('strategies',
      pyndri.krovetz_stem('strategies'),
      pyndri.porter_stem('strategies'))  # strategy, strategi
