import asyncio
import socket
import os
import time
from dotenv import load_dotenv

# --- 导入和配置（SDK 模拟类，保持修正后的结构） ---

class MockStreamSession:
    """模拟 SDK 返回的双向流会话对象 (异步上下文管理器)"""
    async def send_audio(self, chunk):
        """模拟发送音频块，SDK 自动 Protobuf 编码"""
        if len(chunk) > 0:
            # 模拟 SDK 发送数据
            pass
        
    async def __aenter__(self):
        """进入上下文：模拟异步连接建立"""
        await asyncio.sleep(0.1) 
        print("🟢 成功建立 Gemini 流会话。")
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        """退出上下文：模拟关闭流会话"""
        print("👋 SDK 流会话结束。")
        pass

class MockGenerativeStreamClient:
    """模拟 SDK 客户端，用于管理 Gemini Live API 连接"""
    def __init__(self, api_key):
        self.api_key = api_key
        print(f"SDK Client initialized with API Key: {self.api_key[:5]}...")

    # start_stream 是同步方法，返回异步上下文管理器实例
    def start_stream(self, config):
        """返回一个异步上下文管理器实例 (MockStreamSession)"""
        print(f"🔗 SDK 正在建立流式会话 with config: {config}")
        return MockStreamSession()


GenerativeStreamClient = MockGenerativeStreamClient
    
# --- 配置和常量 ---
load_dotenv()
GEMINI_API_KEY = os.getenv("GEMINI_API_KEY") 
if not GEMINI_API_KEY:
    raise ValueError("请在 .env 文件中设置 GEMINI_API_KEY")

ESP32_TCP_PORT = 8888
AUDIO_CHUNK_SIZE = 4096 
AUDIO_CONFIG = {
    "model": "gemini-2.5-flash-live",
    "sample_rate_hertz": 16000,
    "output_sample_rate_hertz": 24000
}

# --- 转发和接收逻辑 ---

async def receive_gemini_responses(stream_session, tcp_socket):
    """
    监听并处理来自 Gemini SDK 的所有回复消息，并将 TTS 音频转发回 ESP32（模拟）。
    """
    loop = asyncio.get_event_loop()
    try:
        # 这是一个模拟循环，模拟 SDK 接收和处理回复
        while True:
            await asyncio.sleep(0.5) 
            
            # 模拟接收到实时文本转录
            # 简化触发条件
            if time.time() % 3 < 0.2:
                print(f"[🎤 STT] 实时识别: 用户正在说话...")
            
            # 模拟接收到完整的文本和 TTS 音频
            # 修正 2: 扩大计时窗口，确保模拟回复能稳定触发
            if time.time() % 5 < 0.5: 
                text = "您好！我是 Gemini，很高兴为您服务。"
                audio_bytes = b'\x00' * 8192 # 模拟 8KB 的 TTS 音频
                
                print(f"\n[🤖 TEXT] Gemini 回复: {text}")
                
                # 将 TTS 音频转发回 ESP32
                try:
                    await loop.sock_sendall(tcp_socket, audio_bytes) 
                    print(f"[🔊 TTS] 转发 {len(audio_bytes)} 字节 TTS 音频回 ESP32...")
                except Exception as e:
                    print(f"转发 TTS 音频失败: {e}")
                    break
            
    except asyncio.CancelledError:
        pass 
    except Exception as e:
        print(f"🚨 接收 Gemini 消息时发生错误: {e}")

async def handle_client_interaction(tcp_socket, addr):
    """
    处理单个 ESP32 连接，并管理与 Gemini SDK 的流式交互。
    """
    client = GenerativeStreamClient(api_key=GEMINI_API_KEY)
    receiver_task = None
    loop = asyncio.get_event_loop()
    
    try:
        async with client.start_stream(config=AUDIO_CONFIG) as stream:
            
            receiver_task = asyncio.create_task(
                receive_gemini_responses(stream, tcp_socket)
            )

            print("🚀 开始转发音频流...")
            
            # 循环接收来自 ESP32 的音频并转发给 SDK
            while True:
                try:
                    audio_chunk = await loop.sock_recv(tcp_socket, AUDIO_CHUNK_SIZE)
                except ConnectionResetError:
                    audio_chunk = b''

                if not audio_chunk:
                    print(f"👋 ESP32 ({addr}) 断开连接，停止音频转发。")
                    break

                # 转发音频数据给 SDK (SDK 自动进行 Protobuf 编码)
                await stream.send_audio(audio_chunk)
                
                # 修正 1: 添加打印，确认收到数据
                #print(f"-> 转发 {len(audio_chunk)} 字节音频到 SDK...")

            
    except Exception as e:
        print(f"🚨 处理客户端 {addr} 时发生致命错误: {e}")
    finally:
        if receiver_task:
            receiver_task.cancel()
        
        tcp_socket.close()
        print(f"连接处理完毕并关闭 ({addr})。")

# --- 主 TCP 服务器启动 ---

async def tcp_server_start():
    """
    启动 TCP 服务器，接受来自 ESP32 的连接。
    """
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.setblocking(False) 
    
    try:
        server_socket.bind(("0.0.0.0", ESP32_TCP_PORT))
        server_socket.listen(5)
        print(f"✨ TCP 音频服务器启动，正在端口 {ESP32_TCP_PORT} 上监听 ESP32 连接...")

        loop = asyncio.get_event_loop()
        while True:
            conn, addr = await loop.sock_accept(server_socket)
            conn.setblocking(False) 
            
            print(f"\n📞 接收到来自 {addr} 的新连接 (ESP32)。")
            
            asyncio.create_task(handle_client_interaction(conn, addr))

    except KeyboardInterrupt:
        print("\n服务器停止。")
    except Exception as e:
        print(f"服务器发生错误: {e}")
    finally:
        server_socket.close()

# --- 运行主程序 ---
if __name__ == "__main__":
    print("--- 启动中介服务器 (基于 Google AI SDK 惯例) ---")
    asyncio.run(tcp_server_start())