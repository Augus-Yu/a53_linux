
# target source
OBJS  := $(SMP_SRCS:%.c=%.o)

CFLAGS += $(COMM_INC)

MPI_LIBS += $(REL_LIB)/libhdmi.a
MPI_LIBS += $(REL_LIB)/libdsp.a
ifeq ($(CONFIG_HI_MOTIONFUSION_SUPPORT),y)
MPI_LIBS += $(REL_LIB)/libmotionfusion.a
endif

.PHONY : clean all

all: $(TARGET)

$(TARGET):$(COMM_OBJ) $(OBJS)
	@$(CC) $(CFLAGS) $(LIBS_LD_CFLAGS) -lpthread -lm -o $(TARGET_PATH)/$@ $^ -Wl,--start-group $(MPI_LIBS) $(SENSOR_LIBS) $(AUDIO_LIBA) $(REL_LIB)/libsecurec.a -Wl,--end-group

clean:
	@rm -f $(TARGET_PATH)/$(TARGET)
	@rm -f $(OBJS)
	@rm -f $(COMM_OBJ)

cleanstream:
	@rm -f *.h264
	@rm -f *.h265
	@rm -f *.jpg
	@rm -f *.mjp
	@rm -f *.mp4
