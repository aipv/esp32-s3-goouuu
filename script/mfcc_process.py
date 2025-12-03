import librosa
import numpy as np
import os
import glob # 用于查找文件路径

# --- 配置 MFCC 参数 (与之前的提取脚本保持一致) ---
SAMPLING_RATE = 16000 # 16000 Hz
N_MFCC = 40          # 40 个 MFCC 系数
# 帧长 (25ms) 和 帧移 (10ms)
N_FFT = int(SAMPLING_RATE * 0.025)  
HOP_LENGTH = int(SAMPLING_RATE * 0.010) 
TARGET_DURATION = 1.2  # 目标时长为 1.2 秒
TARGET_SAMPLES = int(SAMPLING_RATE * TARGET_DURATION) # 19200 采样点

# 预期输出的特征形状，用于验证
EXPECTED_SHAPE = (121, N_MFCC) 

# --- 路径配置 ---
INPUT_DIR = './data/2_extension/'
OUTPUT_DIR = './data/3_mfcc_features/'

def load_pcm_and_standardize(file_path):
    """
    直接读取原始 PCM 16-bit 数据，标准化长度，并转换为浮点数。
    """
    try:
        # 1. 直接从文件读取原始 PCM 16-bit 数据
        audio_int16 = np.fromfile(file_path, dtype='int16')
       
        # 2. 将 16-bit 整数转换为浮点数 [−1.0, 1.0] 范围
        audio = audio_int16.astype(np.float32) / 32768.0 
       
        # 3. 标准化音频长度 (截断或填充)
        if len(audio) < TARGET_SAMPLES:
             padding_needed = TARGET_SAMPLES - len(audio)
             audio = np.pad(audio, (0, padding_needed), 'constant')
        elif len(audio) > TARGET_SAMPLES:
             audio = audio[:TARGET_SAMPLES]

        return audio

    except Exception as e:
        print(f"读取或标准化 PCM 文件失败 {file_path}: {e}")
        return None

def extract_mfcc(audio):
    """
    从标准化后的浮点音频数据中提取 MFCC 特征。
    """
    # 提取 MFCCs
    mfccs = librosa.feature.mfcc(
        y=audio, 
        sr=SAMPLING_RATE, 
        n_mfcc=N_MFCC, 
        n_fft=N_FFT, 
        hop_length=HOP_LENGTH
    )
   
    # 特征标准化 (归一化): 减去均值，除以标准差
    # axis=1 表示沿着时间轴（行）进行操作
    mfccs = (mfccs - np.mean(mfccs, axis=1, keepdims=True)) / np.std(mfccs, axis=1, keepdims=True)
   
    # 形状调整：从 (N_MFCC, 时间帧数) 转置为 (时间帧数, N_MFCC)
    mfccs_transposed = mfccs.T
    
    return mfccs_transposed

# --- 主批量处理函数 ---

def batch_process_mfcc():
    """遍历输入目录中的所有 .bin 文件并提取 MFCCs"""
    
    # 确保输出目录存在
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    # 使用 glob 查找所有 .bin 文件
    input_files = glob.glob(os.path.join(INPUT_DIR, '*.bin'))
    
    if not input_files:
        print(f"错误: 在目录 {INPUT_DIR} 中未找到任何 .bin 文件。请检查路径。")
        return

    print(f"总共找到 {len(input_files)} 个扩增音频文件。开始批量提取 MFCC...")
    
    processed_count = 0
    
    for file_path in input_files:
        filename = os.path.basename(file_path)
        base_name = os.path.splitext(filename)[0]
        
        # 1. 加载和标准化 PCM 数据
        audio = load_pcm_and_standardize(file_path)
        
        if audio is None:
            print(f"跳过文件: {filename}")
            continue
            
        # 2. 提取 MFCC 特征
        mfcc_features = extract_mfcc(audio)
        
        # 3. 形状检查
        if mfcc_features.shape != EXPECTED_SHAPE:
            print(f"警告: 文件 {filename} 的 MFCC 形状不一致！预期 {EXPECTED_SHAPE}，实际 {mfcc_features.shape}")
        
        # 4. 保存特征矩阵为 .npy 文件
        output_file_path = os.path.join(OUTPUT_DIR, f"{base_name}.npy")
        np.save(output_file_path, mfcc_features)
        
        processed_count += 1
        if processed_count % 100 == 0:
            print(f"已处理 {processed_count} 个文件...")
            
    print(f"\n========================================================")
    print(f"✅ 批量 MFCC 提取完成！总共处理了 {processed_count} 个文件。")
    print(f"MFCC 特征已保存到目录: {OUTPUT_DIR}")
    print(f"特征形状 (时间帧数, MFCC系数): {EXPECTED_SHAPE}")
    print(f"========================================================")

if __name__ == '__main__':
    batch_process_mfcc()