#if defined(_MSC_VER)

// Uni.CAN
#include "can_ixxat_channel.h"


namespace Uni::CAN {
    CanChannelIxxat::CanChannelIxxat(uni_can_devinfo_t *devInfo, size_t channelIdx, uint32_t baudrate) {
        _dev_info = *devInfo;
        _channel_idx = channelIdx;
        _can_baudrate = baudrate;
    }

    CanChannelIxxat::~CanChannelIxxat() {
        Close();
        DeInit();
    }

    bool CanChannelIxxat::Close() {
        if (_can_control) {
            _can_control->StopLine();
        }

        threadStop();
        _read_queue.clear();

        return true;
    }

    bool CanChannelIxxat::Open() {
        _read_queue.clear();

        if (!_can_control) {
            return false;
        }

        threadStart();
        _can_control->StartLine();

        return true;
    }

    bool CanChannelIxxat::ReceiveMessage(uni_can_message_t &msg) {
        if (_read_queue.empty()) {
            return false;
        }

        msg = _read_queue.pop();
        return true;
    }

    bool CanChannelIxxat::TransmitMessage(const uni_can_message_t &msg) {
        PCANMSG msg_native = nullptr;
        uint16_t msg_native_cnt = 0;

        if (_can_writer->AcquireWrite((void **) &msg_native, &msg_native_cnt) != VCI_OK) {
            return false;
        }

        if (msg_native_cnt == 0 || !msg_native) {
            _can_writer->ReleaseWrite(0);
            return false;
        }

        msg_native[0].dwMsgId = msg.id;
        msg_native[0].dwTime = 0;
        msg_native[0].uMsgInfo.Bytes.bType = CAN_MSGTYPE_DATA;
        msg_native[0].uMsgInfo.Bytes.bFlags = CAN_MAKE_MSGFLAGS(CAN_LEN_TO_SDLC(msg.len), 0, 0, 0, 1);

        std::memcpy(msg_native->abData, msg.data.u8, msg.len);

        if (_can_writer->ReleaseWrite(1) != VCI_OK) {
            return false;
        }

        return true;
    }

    bool CanChannelIxxat::Init() {
        if (VciGetDeviceManager(&_device_mgr) != VCI_OK) {
            return false;
        }

        VCIID vciid{};
        vciid.AsInt64 = _dev_info.device_index;
        if (_device_mgr->OpenDevice(vciid, &_device) != VCI_OK) {
            return false;
        }

        if (_device->OpenComponent(CLSID_VCIBAL, IID_IBalObject, (void **) &_device_bal) != VCI_OK) {
            return false;
        }

        if (_device_bal->OpenSocket(_channel_idx, IID_ICanSocket, (void **) &_can_socket) != VCI_OK) {
            return false;
        }

        if (_device_bal->OpenSocket(_channel_idx, IID_ICanControl2, (void **) &_can_control) != VCI_OK) {
            return false;
        }

        if (_can_socket->CreateChannel(false, &_can_channel) != VCI_OK) {
            return false;
        }

        if (_can_channel->Initialize(wRxFifoSize, wTxFifoSize) != VCI_OK) {
            return false;
        }

        if (_can_channel->GetReader(&_can_reader) != VCI_OK) {
            return false;
        }

        if (_can_channel->GetWriter(&_can_writer) != VCI_OK) {
            return false;
        }

        _can_reader->SetThreshold(wRxThreshold);
        _can_writer->SetThreshold(wTxThreshold);

        _can_reader_event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        _can_reader->AssignEvent(_can_reader_event);

        if (!InitLine()) {
            return false;
        }

        return true;
    }

    bool CanChannelIxxat::InitLine() {
        if (_can_channel->Activate() != VCI_OK) {
            return false;
        }

        CANBTP baudrate = CAN_BTP_EMPTY;
        switch (_can_baudrate) {
            case 5'000:
                baudrate = CAN_BTP_5KB;
                break;
            case 10'000:
                baudrate = CAN_BTP_10KB;
                break;
            case 20'000:
                baudrate = CAN_BTP_20KB;
                break;
            case 50'000:
                baudrate = CAN_BTP_50KB;
                break;
            case 100'000:
                baudrate = CAN_BTP_100KB;
                break;
            case 125'000:
                baudrate = CAN_BTP_125KB;
                break;
            case 250'000:
                baudrate = CAN_BTP_250KB;
                break;
            case 500'000:
                baudrate = CAN_BTP_500KB;
                break;
            case 800'000:
                baudrate = CAN_BTP_800KB;
                break;
            case 1'000'000:
                baudrate = CAN_BTP_1000KB;
                break;
            default:
                break;
        }

        CANINITLINE2 init_params{};
        init_params.bOpMode = CAN_OPMODE_STANDARD | CAN_OPMODE_EXTENDED | CAN_OPMODE_ERRFRAME;
        init_params.bExMode = CAN_EXMODE_DISABLED;
        init_params.bSFMode = CAN_FILTER_INCL;
        init_params.bEFMode = CAN_FILTER_INCL;
        init_params.dwSFIds = 0;
        init_params.dwEFIds = 0;
        init_params.sBtpSdr = baudrate;
        init_params.sBtpFdr = CAN_BTP_EMPTY;

        if (_can_control->InitLine(&init_params) != VCI_OK) {
            return false;
        }

        if (_can_control->SetAccFilter(CAN_FILTER_STD, CAN_ACC_CODE_ALL, CAN_ACC_MASK_ALL) != VCI_OK) {
            return false;
        }

        if (_can_control->SetAccFilter(CAN_FILTER_EXT, CAN_ACC_CODE_ALL, CAN_ACC_MASK_ALL) != VCI_OK) {
            return false;
        }

        return true;
    }

    bool CanChannelIxxat::DeInit() {
        if (_can_reader) {
            _can_reader->Release();
            _can_reader = nullptr;
        }

        if (_can_writer) {
            _can_writer->Release();
            _can_writer = nullptr;
        }

        if (_can_control) {
            _can_control->StopLine();
            _can_control->Release();
            _can_control = nullptr;
        }

        if (_can_channel) {
            _can_channel->Deactivate();
            _can_channel->Release();
            _can_channel = nullptr;
        }

        if (_can_socket) {
            _can_socket->Release();
            _can_socket = nullptr;
        }

        if (_device_bal) {
            _device_bal->Release();
            _device_bal = nullptr;
        }

        if (_device) {
            _device->Release();
            _device = nullptr;
        }

        if (_device_mgr) {
            _device_mgr->Release();
            _device_mgr = nullptr;
        }

        return true;
    }

    //
    // Thread
    //

    void CanChannelIxxat::threadProc() {
        bool check_ok = false;
        bool check_again = false;

        while (!_threadTerminate) {
            if (!check_again) {
                check_ok = WaitForSingleObject(_can_reader_event, _const_read_timeout_ms) == WAIT_OBJECT_0;
            }

            if (check_ok || check_again) {
                check_again = readProcess();
            }
        }
    }

    bool CanChannelIxxat::threadStop() {
        if (!_thread.joinable()) {
            return false;
        }

        _threadTerminate = true;
        _thread.join();
        return true;
    }

    bool CanChannelIxxat::threadStart() {
        if (_thread.joinable()) {
            return false;
        }

        _threadTerminate = false;
        _thread = std::thread(&CanChannelIxxat::threadProc, this);
        return true;
    }

    //
    // Read
    //
    bool CanChannelIxxat::readProcess() {
        // parameter checking
        if (!_can_reader) {
            return false;
        }

        PCANMSG msgs = nullptr;

        uint16_t msgs_count = 0;
        if (_can_reader->AcquireRead((PVOID *) &msgs, &msgs_count) != VCI_OK) {
            return false;
        }

        for (size_t idx = 0; idx < msgs_count; idx++) {
            uni_can_message_t msg{};
            msg.id = msgs[idx].dwMsgId;
            msg.len = CAN_EDLC_TO_LEN(msgs[idx].uMsgInfo.Bits.dlc);
            std::memcpy(msg.data.u8, msgs[idx].abData, msg.len);
            _read_queue.push(msg);
        }

        _can_reader->ReleaseRead(msgs_count);
        return true;
    }
} // namespace Uni::CAN

#endif
