# ==============================================================================
#  Makefile - BCC466 Tecnicas Metaheuristicas para Otimizacao Combinatoria
#  Compativel com Linux (g++) e Windows (MinGW-w64 / g++).
# ==============================================================================
#
#  Estrutura de pastas do projeto:
#      src/         codigo-fonte (.cpp e .h)
#      instancias/  arquivos de dados das instancias (.TXT)
#      obj/         arquivos objeto (.o) gerados pela compilacao (nao versionar)
#      pcv         executavel final, gerado na raiz do projeto
#
#  Uso:
#      make          -> compila o executavel (Upcv ou Upcv.exe) na raiz
#      make clean    -> remove os arquivos gerados pela compilacao (obj/*.o e o executavel)
#      make run      -> compila (se preciso) e executa o programa
#
#  Requisitos:
#      Linux:   g++ (pacote build-essential)
#      Windows: MinGW-w64 (ex.: via MSYS2 - pacote mingw-w64-x86_64-gcc),
#               com o mingw32-make (ou make) no PATH.
#
#  IMPORTANTE: o programa le os arquivos de instancia com caminhos relativos
#  (ex.: "instancias/C50.TXT" - veja main.cpp), entao ele deve ser executado
#  a partir da RAIZ do projeto (onde este Makefile esta), e nao de dentro de
#  src/ ou obj/. O alvo "make run" ja faz isso corretamente.
# ==============================================================================

TARGET  = pcv
SRC_DIR = src
OBJ_DIR = obj

CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I$(SRC_DIR)
LIBS =

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
HEADERS = $(wildcard $(SRC_DIR)/*.h)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

# Detecta o sistema operacional para ajustar o nome do executavel e os
# comandos de limpeza/criacao de pastas (Linux/macOS x Windows).
ifeq ($(OS),Windows_NT)
    EXECUTAVEL = $(TARGET).exe
    MKDIR_OBJ  = if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
    CLEANCMD   = del /Q $(subst /,\,$(OBJ_DIR))\*.o $(EXECUTAVEL) 2>NUL
else
    EXECUTAVEL = $(TARGET)
    MKDIR_OBJ  = mkdir -p $(OBJ_DIR)
    CLEANCMD   = rm -f $(OBJ_DIR)/*.o $(EXECUTAVEL)
endif

all: $(EXECUTAVEL)

$(EXECUTAVEL): $(OBJECTS)
	@echo 'Ligando o executavel: $@'
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LIBS)
	@echo 'Compilacao concluida: $@'

# Compila cada src/Arquivo.cpp em obj/Arquivo.o. A pasta obj/ e criada
# automaticamente (pre-requisito "order-only", apos o "|") se ainda nao
# existir.
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(OBJ_DIR)
	@echo 'Compilando: $<'
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJ_DIR):
	$(MKDIR_OBJ)

run: all
	./$(EXECUTAVEL)

clean:
	@echo 'Limpando o projeto...'
	$(CLEANCMD)

.PHONY: all run clean
