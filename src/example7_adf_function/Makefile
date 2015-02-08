TARGET = dreams_test7
CXXFLAGS = -std=c++0x

$(TARGET): $(TARGET).o $(TARGET)_adf.o
	g++ -o $(TARGET) $(TARGET).o $(TARGET)_adf.o -ldral -lz

$(TARGET)_adf.cpp: $(TARGET)_adf.xml
	./adf2c <$(TARGET)_adf.xml >$(TARGET)_adf.cpp


clean:
	$(RM) $(TARGET).o
	$(RM) $(TARGET)_adf.o $(TARGET)_adf.cpp
	$(RM) dral_trace.drl.gz
