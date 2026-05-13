#include <iostream>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <iomanip>

//deklaracija nasih matrica
using Matrix = std::vector<std::vector<double>>;
using Vector = std::vector<double>;
using Matrix3D = std::vector<Matrix>;	//3d Matrica kada krenemo da ih spajamo zbog h > 1

Matrix readMatrix(const std::string& filename) {
    Matrix mat; std::ifstream file(filename);
    if (!file.is_open()) return mat;
    std::string line;
    while (std::getline(file, line)) {
        Vector row; std::stringstream ss(line); double val;
        while (ss >> val) { row.push_back(val); }
        if (!row.empty()) { mat.push_back(row); }
    }
    return mat;
}
Vector readVector(const std::string& filename) {
    Vector vec; std::ifstream file(filename);
    if (!file.is_open()) return vec;
    double val; while (file >> val) { vec.push_back(val); }
    return vec;
}
void writeMatrix(const std::string& filename, const Matrix& mat) {
    std::ofstream file(filename);
    for (const auto& row : mat) {
        for (size_t i = 0; i < row.size(); ++i) {
            file << row[i] << (i == row.size() - 1 ? "" : " ");
        }
        file << std::endl;
    }
}


void analyze_bits(const std::string& name, const Matrix& mat) {
	double min_abs_val = 1e9;
	double max_abs_val = -1e9;
	size_t rows = mat.size();
	size_t cols = mat[0].size();
	
	for (size_t i = 0; i < rows; i++) {
		for (size_t j = 0; j < cols; j++) {
			double val = mat[i][j];
			double abs_val = std::abs(val);
			if (abs_val > max_abs_val) {
				max_abs_val = abs_val;
			}
		}
	}
	//sada radimo formulu log2(max_abs_val)
	int integer_bits = 0;
	if (max_abs_val > 0) {
		integer_bits = static_cast<int>(std::ceil(std::log2(max_abs_val)));
	} else {
		integer_bits = 1; //za nulu
	}
	
	integer_bits += 1; //+1 za znak
	
	//za izlomljeni deo
	int fract_bits = 0;
	
	for (size_t i = 0; i < rows; i++) {
		for (size_t j = 0; j <  cols; j++) {
			double val = min_abs_val;
			double abs_val = std::abs(val);
			if (abs_val != 0 && abs_val < min_abs_val) {
				min_abs_val = abs_val;
			}
		}
	}
	//sada formula
	if (min_abs_val > 0) {
		fract_bits = static_cast<int>(std::ceil(std::log(1.0/min_abs_val)));
	} else {
		fract_bits = 0;
	}
	
	fract_bits += 1;
	
	//ovde samo ispis kao npr:
	std::cout << "Opseg : " << min_abs_val << " ... " << max_abs_val << std::endl;
	
	
}

/*Implementiramo Attention algoritam ovde
Prvo mnozimo matrice Q *  K i zatim Softmax, pa skaliranje...
*/

//mnozenje matrica A x B red po red
Matrix matmul_standard(const Matrix& A, const Matrix& B) {
	size_t A_rows = A.size();
	size_t B_rows = B.size();
	size_t A_cols = A[0].size();
	size_t B_cols = B[0].size();
	Matrix C(A_rows, Vector(B_cols, 0.0));
	for (size_t i = 0; i < A_rows; i++) {
		for (size_t j = 0; j < B_cols; j++) {
			double sum = 0.0;
			for (size_t k = 0; k < A_cols; k++) {
				sum += A[i][k] * B[k][j]; 
			}
			C[i][j] = sum;
		}
	}
	return C;
}

//Transponujemo matricu i mnozimo
Matrix matmul_transpone(const Matrix& A, const Matrix& B) {
	size_t A_rows = A.size();
	size_t B_rows = B.size();
	size_t A_cols = A[0].size();
	size_t B_cols = B[0].size();
	Matrix C(A_rows, Vector(B_rows, 0.0));
	for (size_t i = 0; i < A_rows; i++) {
		for(size_t j = 0; j < B_rows; j++) {
			double sum = 0.0;
			for (size_t k = 0; k < A_rows; k++) {
					sum += A[i][k] * B[j][k];
			}
		C[i][j] = sum; 
		}
	}
	return C;
}

//Prelazimo na Softmax deo
Matrix softmax(const Matrix& mat) {
	size_t rows = mat.size();
	size_t cols = mat[0].size();
	Matrix results = mat;
	for (size_t i = 0; i < rows; i++) {
		double max_val = mat[i][0];
		for (size_t k = 1; k < cols; k++) {
			if (max_val < mat[i][k]) {
				max_val = mat[i][k];
			}
		}
		double sum = 0.0; //promenljiva u koju cu da skladistim zbir elemenata
		for (size_t j = 0; j < cols; j++) {
			results[i][j] = std::exp(mat[i][j] - max_val);
			sum += results[i][j];	//u results je smestena vrednost svakog elementa,  eksponencijalno
		}
		double sum_podeljena = 1/sum;	//ustedim vreme prilikom deljenja kod FPGA
		for (size_t j = 0; j < cols; j++) {
			results[i][j] = results[i][j] * sum_podeljena;
		}
	}
	return results;
}

//ovde pozivamo whisperAI i prosledjujemo matrice i broj glava
//on treba da nam vrati novu matricu koju ce whisperAI enkodovati
Matrix multihead_attention(const Matrix& Q, const Matrix& K, const Matrix& V, int num_heads) {
	size_t seq_len = Q.size(); //matrice su indenticne pa je dovoljno da uzmemo vrednosti samo ove
	size_t embed_dim = Q[0].size();
	size_t head_dim = embed_dim / num_heads; //dimenzija glave, vrv oko 500 ovde, zab sam koliko je embed_dim
	
	Matrix3D q_heads(num_heads, Matrix(seq_len, Vector(embed_dim)));
	Matrix3D k_heads(num_heads, Matrix(seq_len, Vector(embed_dim)));
	Matrix3D v_heads(num_heads, Matrix(seq_len, Vector(embed_dim)));
	//splitujemo glave matrice, tkao da svaku matricu podelimo na 8 glava, po 64 elemenata po koloni koje treba obraditi
	for (int h = 0; h < num_heads; h++) {
		for(size_t i = 0; i < seq_len; i++) {
			for (size_t j = 0; j < head_dim; j++) {
				q_heads[h][i][j] = Q[i][h * head_dim + j];
				k_heads[h][i][j] = K[i][h * head_dim + j];
				v_heads[h][i][j] = V[i][h * head_dim + j];
			}
		}
	}
	
	Matrix3D spojeno(num_heads, Matrix(seq_len, Vector(head_dim)));
	
	//primenjujemo operacija sada nad matricama koje su ponedeljene po glavama
	for (int h = 0; h < num_heads; h++) {
		//prvo mnozimo matrice i dobijamo neki rezultat
		Matrix scores = matmul_transpone(q_heads[h], k_heads[h]);
		//onda radimo softmax za rezultate koje smo dobili
		Matrix probs = softmax(scores);
		//na kraju dobijamo output koji mnozimo sa matricom V
		spojeno[h] = matmul_standard(probs, v_heads[h]); //kada napisemo [h], uzecemo samo matricu u toj glavi, odnosno, 2D matricu.
	}
	
	//sada za merge sve vrednosti matrica iz svake glave mergujemo u jednu 2D matricu
	Matrix merged_output(seq_len, Vector(embed_dim));
		for (int h = 0; h < num_heads; h++) {
			for (size_t i = 0; i < embed_dim; i++) {
				for (size_t j = 0; j < head_dim; j++) {
					merged_output[i][h * head_dim + j] = spojeno[h][i][j];	//ovde dobijamo 2D matricu
				}
			}
		}
		
	return merged_output;
}

int main() {
    //citamo ulazne matrice
    std::cout << "Pokrecemo nas cpp algoritam..." << std::endl;
    Matrix Q = readMatrix("izlaz/matrice/multihead_ulaz_Q.txt");
    Matrix K = readMatrix("izlaz/matrice/multihead_ulaz_K.txt");
    Matrix V = readMatrix("izlaz/matrice/multihead_ulaz_V.txt");
    Matrix W = readMatrix("izlaz/matrice/multihead_W_out.txt");
    Vector b = readVector("izlaz/matrice/multihead_b_out.txt");
	   
    if (Q.empty()) return 1;

	Matrix merged = multihead_attention(Q, K, V, 8);
	//finalna projekcija
	Matrix final_proj = matmul_standard(merged, W);
	size_t seq_len = final_proj.size();
	size_t embed_dim = final_proj[0].size();
	for (int i = 0; i < seq_len; i++) {
		for (int j = 0; j < embed_dim; j++) {
			final_proj[i][j] += b[j]; 
		}
	}
	std::cout << "Finalna projekcija uspesno izvrsena!" << std::endl;
	writeMatrix("izlaz/izlaz_cpp.txt", final_proj);
	std::cout << "Izlazni fajl kreiran." << std::endl;
	return 0;
}
