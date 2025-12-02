import librosa
import numpy as np
import os

# --- 配置 MFCC 参数 (与模型训练一致) ---
SAMPLING_RATE = 16000 # 16000 Hz
N_MFCC = 40          # 40 个 MFCC 系数
N_FFT = int(SAMPLING_RATE * 0.025)  # 400 采样点 (25ms 帧长)
HOP_LENGTH = int(SAMPLING_RATE * 0.010) # 160 采样点 (10ms 帧移)
TARGET_DURATION = 1.2  # 目标时长为 1.2 秒
TARGET_SAMPLES = int(SAMPLING_RATE * TARGET_DURATION) # 19200 采样点

# 理论上的时间帧数 (Time Steps)
# T = (19200 - 400) // 160 + 1 = 118
THEORETICAL_TIME_STEPS = 118

def extract_mfcc(file_path, target_samples=TARGET_SAMPLES):
    """
    加载WAV文件，标准化长度到 1.2 秒 (19200 采样点)，并提取MFCC特征。
    """
   
    print(f"--- 正在处理文件: {os.path.basename(file_path)} ---")
   
    try:
        # 1. 加载音频数据
        # 确保使用 TARGET_SAMPLES 来截断/填充，从而保证 len(audio) == target_samples
        audio, sr = librosa.load(file_path, sr=SAMPLING_RATE)
       
        print(f"原始采样点数: {len(audio)}, 目标采样点数: {target_samples}")
       
        # 2. 标准化音频长度 (截断或填充)
        if len(audio) < target_samples:
             padding_needed = target_samples - len(audio)
             audio = np.pad(audio, (0, padding_needed), 'constant')
             print(f"音频太短，填充了 {padding_needed} 个静音采样点。")
        elif len(audio) > target_samples:
             audio = audio[:target_samples]
             print(f"音频太长，截断到 {target_samples} 个采样点。")

        # 3. 提取 MFCCs
        mfccs = librosa.feature.mfcc(
            y=audio, 
            sr=sr, 
            n_mfcc=N_MFCC, 
            n_fft=N_FFT, 
            hop_length=HOP_LENGTH
        )
       
        # 4. 特征标准化 (归一化): 减去均值，除以标准差
        # axis=1 表示沿着时间轴进行统计
        mfccs = (mfccs - np.mean(mfccs, axis=1, keepdims=True)) / np.std(mfccs, axis=1, keepdims=True)
       
        # 5. 形状调整：从 (N_MFCC, 时间帧数) 转置为 (时间帧数, N_MFCC)
        mfccs_transposed = mfccs.T
       
        print(f"输出 MFCC 形状: {mfccs_transposed.shape}")
       
        return mfccs_transposed

    except Exception as e:
        print(f"处理文件失败 {file_path}: {e}")
        return None

# --- 示例使用 ---
file_to_process = '2.wav' 

if not os.path.exists(file_to_process):
    print(f"\n错误：找不到文件 '{file_to_process}'。请将您的WAV文件放在此路径下进行测试。")
else:
    mfcc_features = extract_mfcc(file_to_process)
   
    if mfcc_features is not None:
        # 最终形状应该是 (118, 40)
        print("\nMFCC 特征提取成功！")
        print(f"最终 MFCC 矩阵形状: {mfcc_features.shape}")