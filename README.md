# TeslaMetr v18.05 by Verloka Vadim, 2018

*Задача:* Разработать модуль измеряющий магнитную индукцию в Гауссах для поключения к микроконтроллеру AVR-microlab.

*Используемый датчик:* SS443

*Чип:* ATmega32 16 MHz

*Версия прошивки:* 18.05

*Интерфейс связи с ПК:* RS232

## Принципиальная схема модуля
[![Logo](https://raw.githubusercontent.com/ogycode/TeslaMetr/master/merch/cheme.PNG)]()

Так, как это университетский проект то все предоставленные комплектующие имеют минимальную стоимость, а некоторые еще и "советского" происхождения. **Немного про датчик.** SS443 - самый дешевый датчик который можно только найти, так вот, это индикаторный датчик, т.е. реагирует на смену полюсов, про измерения в даташите нет и единого слова, а задача стояла следующая: разработать модуль для **ИЗМЕРЕНИЯ** М-поля. Исходя из этого возникли проблемы с зависимостью датчика от напряжения питания.

## График зависимости тока
[![Logo](https://raw.githubusercontent.com/ogycode/TeslaMetr/master/merch/graph1.PNG)]()

Для измерений мне предоставили соленоид с *К = 8000*. Исходя из этого удалось получить зависимость погрешности от М-поля

## График погрешности показаний от тока
[![Logo](https://raw.githubusercontent.com/ogycode/TeslaMetr/master/merch/graph2.PNG)]()

После некоторых манипуляций с коэфициентами и преобразований кода АЦП удалось добиться адекватного измерения.

## График измереной индукции и расчитаной индукции
[![Logo](https://raw.githubusercontent.com/ogycode/TeslaMetr/master/merch/graph3.PNG)]()

## Режимы работы
  - Установка нуля
  - Измерение и отображение (с отсылкой на ПК)
  - Измерение и отображение (с отсылкой на ПК | без преобразований кода АЦП)
  - Режим максимального быстроействия (прилож. измеряет указаное в прошивке количество точек с максимальным быстродействием, без траты времени на ЖК-индикацию и отправку на ПК, после измерения массив данных отсылается на ПК)
  - Отображения напряжения питания датчика
  - Отображения напряжения питания датчика без преобразования кода АЦП

# PS
Идиотский проект, как и все остальное гос. образование снг региона. 

*' - Эй, юнец, разработай мне велосипед. Ах да, oформи ка мне все это по ЕСКД'*