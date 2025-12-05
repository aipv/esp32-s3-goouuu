import numpy as np
import os
import sys
import random
from scipy.io import wavfile

# --- 配置参数 ---
SAMPLE_RATE = 16000  # 采样率
SAMPLE_WIDTH = 2     # 16-bit = 2 bytes
KEYWORD_LENGTH = SAMPLE_RATE # 1秒的样本长度

# 输入目录和文件
RAW_DIR = 'data/0_raw_collection/'
NOISE_FILE = os.path.join(RAW_DIR, '00_background/noise.pcm')
CONFUSION_DIR = os.path.join(RAW_DIR, '02_confusion/')
DAILY_DIR = os.path.join(RAW_DIR, '03_daily/')

# 输出目录
OUTPUT_DIR = 'data/1_training_sample/11_negative/'

# 扩增配置
SNR_RANGE_DB = [-5, 0, 5, 10, 15] # 目标信噪比范围 (dB)
AUGMENTATION_COUNT_PER_SAMPLE = 5 # 每个语音负样本混合次数

# --- 辅助函数：计算功率和调整增益 ---

def calculate_rms(audio_data):
    """计算音频数据的均方根值 (RMS), 代表信号强度/功率."""
    audio_data = audio_data.astype(np.float64) 
    return np.sqrt(np.mean(audio_data**2))

def adjust_gain_for_snr(signal, noise_segment, snr_db):
    """调整噪音片段的增益，使其与信号片段达到指定的信噪比 (SNR)。"""
    rms_signal = calculate_rms(signal)
    rms_noise = calculate_rms(noise_segment)
    
    if rms_noise == 0 or rms_signal == 0:
        return np.zeros_like(noise_segment, dtype=signal.dtype)
    
    linear_power_ratio = 10**(snr_db / 10.0)
    target_rms_noise = rms_signal / np.sqrt(linear_power_ratio)
    gain_factor = target_rms_noise / rms_noise
    
    adjusted_noise = noise_segment * gain_factor
    return adjusted_noise.astype(signal.dtype)

def process_and_augment_speech(speech_dir, noise_raw, output_dir, label):
    """处理并混合语音负样本 (混淆词或日常语音)。"""
    
    speech_files = sorted([f for f in os.listdir(speech_dir) if f.endswith('.pcm')])
    count = 0
    max_start = len(noise_raw) - KEYWORD_LENGTH
    
    print(f"\n--- 开始处理 {label} 负样本 ({len(speech_files)} 个原始文件) ---")

    for filename in speech_files:
        speech_path = os.path.join(speech_dir, filename)
        base_name = os.path.splitext(filename)[0]
        
        try:
            speech_data = np.fromfile(speech_path, dtype=np.int16)
        except FileNotFoundError:
            continue
            
        if len(speech_data) != KEYWORD_LENGTH:
             # 如果录音不是恰好 1 秒，可以选择截断或填充，这里为了简单直接跳过
             continue

        for j in range(AUGMENTATION_COUNT_PER_SAMPLE):
            
            # 1. 随机选择一个 SNR 值
            snr_db = random.choice(SNR_RANGE_DB)
            
            # 2. 从噪音文件中随机选择一个 1秒 的片段
            start_index = random.randint(0, max_start)
            noise_segment = noise_raw[start_index : start_index + KEYWORD_LENGTH]
            
            # 3. 调整噪音片段的增益并混合
            adjusted_noise = adjust_gain_for_snr(speech_data, noise_segment, snr_db)
            mixed_data_float = speech_data.astype(np.float64) + adjusted_noise.astype(np.float64)
            
            # 4. 限制范围并保存
            mixed_data = np.clip(mixed_data_float, -32768, 32767).astype(np.int16)
            
            output_path_aug = os.path.join(output_dir, f'{label}_{base_name}_AUG_{j:02d}_SNR{snr_db}.pcm')
            mixed_data.tofile(output_path_aug)
            count += 1
            
    print(f"✅ {label} 混合完成，共生成 {count} 个带噪样本。")
    return count

def generate_negative_samples():
    """执行负样本的生成和扩增。"""
    
    # 1. 确保输出目录存在
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # 2. 读取背景噪音数据
    print(f"读取背景噪音文件: {NOISE_FILE}...")
    try:
        noise_raw = np.fromfile(NOISE_FILE, dtype=np.int16)
    except FileNotFoundError:
        print(f"错误: 找不到噪音文件 {NOISE_FILE}。请检查路径。")
        return
        
    total_samples = 0
    
    # --- I. 切割纯噪音/静音样本 ---
    print("\n--- I. 切割纯噪音/静音样本 ---")
    
    # 25 分钟 * 60 秒/分钟 = 1500 秒
    available_seconds = len(noise_raw) // KEYWORD_LENGTH
    
    for i in range(available_seconds):
        start_index = i * KEYWORD_LENGTH
        noise_segment = noise_raw[start_index : start_index + KEYWORD_LENGTH]
        
        output_path_noise = os.path.join(OUTPUT_DIR, f'NOISE_{i:04d}.pcm')
        noise_segment.tofile(output_path_noise)
        total_samples += 1
        
    print(f"✅ 纯噪音/静音样本切割完成，共生成 {total_samples} 个样本。")
    
    # --- II. 混合混淆词负样本 (Confusion) ---
    count_confusion = process_and_augment_speech(CONFUSION_DIR, noise_raw, OUTPUT_DIR, 'CONF')
    total_samples += count_confusion
    
    # --- III. 混合日常语音负样本 (Daily) ---
    count_daily = process_and_augment_speech(DAILY_DIR, noise_raw, OUTPUT_DIR, 'DAILY')
    total_samples += count_daily

    print("\n--- 负样本扩增完成 ---")
    print(f"最终负样本集总计文件数: {total_samples}")
    print(f"文件已保存至: {OUTPUT_DIR}")


if __name__ == '__main__':
    generate_negative_samples()