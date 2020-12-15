# SOP-P4
Zadanie polega na napisaniu programu przeprowadzającego prostą symulację wyścigów wątków. Program przyjmuje różne komendy, które mogą manipulować stanem wyścigu oraz wyświetlać ten stan.

TODO:
0. utworz struktury (GLOBALNE - najwyzej cie poprawia)
	- participant
	- race
1. wstepna inicjalizacja
	- pobieranie info z argumentow main (w main)
	- powitanie uzytkownika
	- odbiernanie od uzytkownika komend
2. rozpoczynanie wyscigu
	- losowanie srednich predkosci (mozna w watku glownym)
	- przekazywanie srednich predkosci do watkow
3. przebieg wyscigu (od strony workera)
	- spanie watkow przez sekunde (zrob sleepem, bez zastanawiania sie, najwyzej cie poprawia)
	- wykrywanie wykonania okrazenia
	- wysylanie SIGUSR1 do watku glownego
4. przebieg wyscigu (od strony maina)
	- oczekiwanie na sygnal SIGUSR1
	- odbieranie przez watek glowny SIGUSR1
	- wyswietlanie informacji (poza handlerem) (SPORE)
5. koniec wyscigu
	- zabijanie watkow (wewnatrz poszczegolnych watkow)
	- wyswietlanie posortowanej tabeli wynikow
6. obsluga komend (jak nie jest napisane ze moze dzialac podczas wyscigu to nie moze!)
7. osobny watek na obsluge sygnalow
	- oczekiwanie na sygnal (odblokuj)
	- blokowanie sygnalu w pozostalych watkach (zablokuj przed rozmnazaniem watkow)
	- wyswietlanie pytania
8. bezpieczenstwo i poprawnosc (NA KONIEC!)
	- porob muteksy dla danych wykorzystywanych zarowno przez uczerstnikow jak i program glowny
9. pytania do prowadzacych
	- zapytac o komendy co do ktorych nie jestes pewien czy maja byc dostepne podczas wyscigu
	- globalne struktury OK?
