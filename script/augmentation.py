import numpy as np
import os
import random

# --- 配置参数 ---
SAMPLING_RATE = 16000  # 采样率
TARGET_DURATION = 1.2  # 目标时长（秒），与您的 MFCC 提取一致
TARGET_SAMPLES = int(SAMPLING_RATE * TARGET_DURATION) # 19200 采样点

# --- 文件路径配置 ---
NOISE_FILE_PATH = 'data/0_noise/noise_pcm16.bin'           
OUTPUT_DIR = 'data/2_extension/'

# --- 扩增版本数量 ---
NOISE_VERSIONS = 5  # 叠加 5 个不同信噪比的版本
VOLUME_VERSIONS = 3 # 改变 3 个不同音量的版本
SHIFT_VERSIONS = 2  # 增加 2 个时间偏移版本

def get_next_filename():
    n = 1
    while os.path.exists(f"{n}.wav"):
        n += 1
    return f"data/1_raw/{n}.wav"


# --- 核心 I/O 函数：读取和保存原始 PCM 16-bit ---

def load_pcm(file_path):
    """
    直接从文件读取原始 PCM 16-bit 数据，并转换为 [-1.0, 1.0] 的 float32 数组。
    """
    if not os.path.exists(file_path):
        print(f"错误：找不到文件 {file_path}。")
        return None
        
    # 读取 16-bit 整数数据
    audio_int16 = np.fromfile(file_path, dtype='int16')
    
    # 转换为浮点数 [−1.0, 1.0] 范围
    audio_float = audio_int16.astype(np.float32) / 32768.0 
    return audio_float

def save_pcm(file_path, audio_float):
    """
    将 [-1.0, 1.0] 的 float32 数组转换回 PCM 16-bit 整数并保存到文件。
    """
    # 裁剪到 [-1.0, 1.0] 范围，防止溢出
    audio_float[audio_float > 1.0] = 1.0
    audio_float[audio_float < -1.0] = -1.0
    
    # 转换回 16-bit 整数
    audio_int16 = (audio_float * 32767.0).astype(np.int16)
    
    # 保存为原始二进制数据
    audio_int16.tofile(file_path)

# --- 核心辅助函数：扩增逻辑 (基于 float32 数据) ---

def standardize_length(audio):
    """确保音频长度与 TARGET_DURATION 一致"""
    if len(audio) < TARGET_SAMPLES:
        audio = np.pad(audio, (0, TARGET_SAMPLES - len(audio)), 'constant')
    elif len(audio) > TARGET_SAMPLES:
        audio = audio[:TARGET_SAMPLES]
    return audio

def apply_noise_mixing(y, noise_full, snr_range=[5, 20]):
    """叠加随机信噪比 (SNR) 的背景噪声"""
    if len(noise_full) < len(y):
        noise_full = np.tile(noise_full, int(np.ceil(len(y) / len(noise_full))))

    start = random.randint(0, len(noise_full) - len(y))
    noise_segment = noise_full[start : start + len(y)]

    snr_db = random.uniform(snr_range[0], snr_range[1])
    
    signal_power = np.mean(y ** 2)
    noise_power = np.mean(noise_segment ** 2)
    
    # 确保信号和噪声功率非零以避免除零错误
    if signal_power == 0 or noise_power == 0:
        return y # 如果静音，则不叠加
    
    target_noise_power = signal_power / (10**(snr_db / 10))
    noise_scaling_factor = np.sqrt(target_noise_power / noise_power)
    
    mixed_audio = y + noise_segment * noise_scaling_factor
    
    # 归一化，防止削波 (Clipping)
    max_val = np.max(np.abs(mixed_audio))
    if max_val > 1.0:
        mixed_audio = mixed_audio / max_val
        
    return mixed_audio

def apply_volume_perturbation(y, scale_range=[0.4, 1.5]):
    """改变音量（强度）"""
    scale = random.uniform(scale_range[0], scale_range[1])
    y_scaled = y * scale
    
    return y_scaled # 不在扩增函数内裁剪，交由 save_pcm 统一处理

def apply_time_shift(y, shift_range=[-0.1, 0.1]):
    """将音频在时间轴上微小移动（秒）"""
    max_shift_samples = int(SAMPLING_RATE * random.uniform(shift_range[0], shift_range[1]))
    
    y_shifted = np.roll(y, max_shift_samples)
    
    if max_shift_samples > 0:
        y_shifted[:max_shift_samples] = 0.0 
    elif max_shift_samples < 0:
        y_shifted[max_shift_samples:] = 0.0 
        
    return y_shifted

def process_one_file(input_file): 
    # 2. 加载原始音频和噪声文件 (都是 PCM)
    y_raw_float = load_pcm(input_file)
    noise_full_float = load_pcm(NOISE_FILE_PATH)
    
    if y_raw_float is None or noise_full_float is None:
        print("\n无法加载输入或噪声文件。请检查路径和文件名。")
        exit()

    # 3. 标准化原始音频长度
    y_raw = standardize_length(y_raw_float)
    
    print(f"原始文件已加载并标准化长度到 {TARGET_DURATION} 秒 ({len(y_raw)} 采样点)。")
    
    base_name = os.path.splitext(os.path.basename(input_file))[0]
    total_generated = 0

    # --- 原始文件 (作为纯净样本保存) ---
    output_path_clean = os.path.join(OUTPUT_DIR, f"{base_name}_00_clean.bin")
    save_pcm(output_path_clean, y_raw)
    total_generated += 1
    
    # --- 噪声叠加 ---
    print(f"正在生成 {NOISE_VERSIONS} 个噪声叠加版本...")
    for i in range(NOISE_VERSIONS):
        y_aug = apply_noise_mixing(y_raw, noise_full_float)
        output_path = os.path.join(OUTPUT_DIR, f"{base_name}_01_noise_{i+1}.bin")
        save_pcm(output_path, y_aug)
        total_generated += 1
        
    # --- 音量变化 ---
    print(f"正在生成 {VOLUME_VERSIONS} 个音量变化版本...")
    for i in range(VOLUME_VERSIONS):
        y_aug = apply_volume_perturbation(y_raw)
        output_path = os.path.join(OUTPUT_DIR, f"{base_name}_02_vol_{i+1}.bin")
        save_pcm(output_path, y_aug)
        total_generated += 1
        
    # --- 时间偏移 ---
    print(f"正在生成 {SHIFT_VERSIONS} 个时间偏移版本...")
    for i in range(SHIFT_VERSIONS):
        y_aug = apply_time_shift(y_raw)
        output_path = os.path.join(OUTPUT_DIR, f"{base_name}_03_shift_{i+1}.bin")
        save_pcm(output_path, y_aug)
        total_generated += 1

    print(f"\n=========================================================")
    print(f"✅ 扩增完成！共生成 {total_generated} 个新样本。")
    print(f"文件已保存到目录: {OUTPUT_DIR} (格式：原始 PCM 16-bit)。")
    print(f"=========================================================")


# --- 主程序 ---
if __name__ == '__main__':
    
    # 1. 准备输出目录
    os.makedirs(OUTPUT_DIR, exist_ok=True)

    for i in range(1, 201):
        input_file = f"data/1_raw/{i}.wav"
        process_one_file(input_file)
